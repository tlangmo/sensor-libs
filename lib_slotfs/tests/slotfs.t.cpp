// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "../../unittest/catch.hpp"
#include "slotfs.h"
#include <array>
#include <vector>
#include <algorithm>
#include <assert.h>
using namespace motesque;
using namespace slotfs;

template<size_t BLOCK_SIZE, size_t BLOCK_COUNT>
class BlockDeviceMock : public BlockDevice
{
public:
    virtual ~BlockDeviceMock()
    {

    }
    BlockDeviceMock() : blocks() {
        std::for_each(blocks.begin(),blocks.end(), [](std::array<uint8_t,BLOCK_SIZE>& block) {
            std::fill(block.begin(),block.end(),0);
        });
    }

    int write(uint64_t start_address, const uint8_t* data, uint64_t size)
    {
        assert(size % BLOCK_SIZE == 0);
        if (size == 0) {
            return Result_Success;
        }
        for (uint64_t cur_address = start_address;
                cur_address < start_address+size;
                cur_address+= BLOCK_SIZE, data+=BLOCK_SIZE) {
            size_t block_idx = cur_address / BLOCK_SIZE;
            size_t offset = cur_address % BLOCK_SIZE;
            if (block_idx >= BLOCK_COUNT || offset != 0) {
                return Result_Error_Write;
            }

            std::copy(data, data + size, blocks[block_idx].begin());
        }
        return Result_Success;
    }

    int read(uint64_t start_address, uint8_t* data, uint64_t size)
    {
        assert(size % BLOCK_SIZE == 0);
        for (uint64_t cur_address = start_address;
                cur_address < start_address+size;
                cur_address+= BLOCK_SIZE, data+=BLOCK_SIZE) {
            size_t block_idx = cur_address / BLOCK_SIZE;
            size_t offset = cur_address % BLOCK_SIZE;
            if (block_idx >= BLOCK_COUNT || offset != 0) {
                return Result_Error_Read;
            }
            std::copy(blocks[block_idx].begin(), blocks[block_idx].begin() + BLOCK_SIZE, data);
        }
        return Result_Success;
    }
    std::array< std::array<uint8_t, BLOCK_SIZE>, BLOCK_COUNT > blocks;
};

TEST_CASE("sfs init")
{
    SlotFS sfs;
    BlockDeviceMock<512,1000> bd;
    Config cfg;
    cfg.slot_count = 10;
    cfg.start_address = 0;
    cfg.end_address = 512*100;
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(rc == Result_Success);
    REQUIRE(sfs.config.slot_count == 10);
    REQUIRE(sfs.config.start_address == 0);
    REQUIRE(sfs.config.end_address == 512*100);

    // test that the config is written to disk
    Config cfg_on_disk = *(Config*)(bd.blocks[0].data());
    REQUIRE(sfs.config.slot_count == cfg_on_disk.slot_count);
    REQUIRE(sfs.config.start_address == cfg_on_disk.start_address);
    REQUIRE(sfs.config.end_address== cfg_on_disk.end_address);

    MetaDataBlock md_on_disk = *(MetaDataBlock*)(bd.blocks[1].data());
    REQUIRE(md_on_disk.revision == 0);
    REQUIRE(md_on_disk.start_address == cfg.slot_count*512*2+512);
    uint64_t slot_size =  (cfg.end_address-md_on_disk.start_address)/cfg.slot_count;
    REQUIRE(md_on_disk.max_address > md_on_disk.start_address);
    REQUIRE(md_on_disk.file_size == 0);
    // the second meta data block for slot 0 should match the first
    MetaDataBlock md_on_disk_b = *(MetaDataBlock*)(bd.blocks[2].data());
    REQUIRE(memcmp(&md_on_disk, &md_on_disk_b,sizeof(MetaDataBlock)) == 0);

    REQUIRE( 0 == sfs_deinit(&sfs));
    REQUIRE(sfs.config.slot_count == 0);
    REQUIRE(sfs.block_device == nullptr);
}

TEST_CASE("sfs init with formatting")
{
    SlotFS sfs;
    BlockDeviceMock<512,1000> bd;
    Config cfg;
    cfg.slot_count = 10;
    cfg.start_address = 0;
    cfg.end_address = 512*100;
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(rc == Result_Success);
    // an init on an formated disk should not change the meta_data blocks
    bd.blocks[1].data()[0] = 0xcd;
    rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(bd.blocks[1].data()[0] == 0xcd);
}



TEST_CASE("most_recent_meta_data_block")
{
    SECTION("consecutive revision") {
        MetaDataBlock md_a;
        MetaDataBlock md_b;
        md_a.revision = 10;
        md_b.revision = 11;
        size_t idx = 0;
        REQUIRE( Result_Success == most_recent_meta_data_block(md_a, true, md_b,true, &idx));
        REQUIRE( idx == 1);
        REQUIRE( Result_Success == most_recent_meta_data_block(md_a, true, md_b, false, &idx));
        REQUIRE( idx == 0);
        REQUIRE( Result_Success == most_recent_meta_data_block(md_a, false, md_b, true, &idx));
        REQUIRE( idx == 1);
        REQUIRE( Result_Error_CorruptData == most_recent_meta_data_block(md_a, false, md_b, false, &idx));
    }

    SECTION("overflowed revision") {
        MetaDataBlock md_a;
        MetaDataBlock md_b;
        md_a.revision = 10;
        md_b.revision = 2;
        size_t idx = 0;
        REQUIRE( Result_Success == most_recent_meta_data_block(md_a, true, md_b,true, &idx));
        REQUIRE( idx == 1);
    }
}

TEST_CASE("sfs file size")
{
    SlotFS sfs;
    BlockDeviceMock<512,1000> bd;
    Config cfg;
    cfg.slot_count = 10;
    cfg.start_address = 0;
    cfg.end_address = 512*100;
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(rc == Result_Success);
    SlotFS::File file;
    rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file);
    REQUIRE( Result_Success == rc);

    size_t file_size;
    rc = sfs_file_size(&file,&file_size);
    REQUIRE( Result_Success == rc);
    REQUIRE( file_size == 0);

    size_t written = 0;
    int data = 0;
    rc = sfs_file_write(&sfs, &file, (uint8_t*)&data, sizeof(data), &written);
    REQUIRE( Result_Success == rc);
    REQUIRE( sizeof(data) == written);

    // the file size changes only after flush (meta data block)s
    rc = sfs_file_size(&file,&file_size);
    REQUIRE( file_size == 0);

    sfs_file_flush(&sfs, &file);
    rc = sfs_file_size(&file,&file_size);
    REQUIRE( file_size == 4);


}



TEST_CASE("sfs open/close Mode_WriteCreate")
{
    SlotFS sfs;
    BlockDeviceMock<512,1000> bd;
    Config cfg;
    cfg.slot_count = 10;
    cfg.start_address = 0;
    cfg.end_address = 512*100;
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(rc == Result_Success);

    SlotFS::File file;
    rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file);
    REQUIRE( Result_Success == rc);
    REQUIRE(sfs.lock_counters[0] == 1);

    SECTION("open same file") {
        rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file);
        REQUIRE( Result_Error_Assert == rc);
        REQUIRE(sfs.lock_counters[0] == 1);
    }

    SECTION("open same slot") {
        SlotFS::File file2;
        rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file2);
        REQUIRE( Result_Error_Assert == rc);
        REQUIRE(sfs.lock_counters[0] == 1);
    }

    SECTION("open same file after close") {
        sfs_file_close(&sfs, &file);
        REQUIRE(sfs.lock_counters[0] == 0);
        rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file);
        REQUIRE( Result_Success == rc);
        REQUIRE(sfs.lock_counters[0] == 1);
    }
}

TEST_CASE("sfs open/close Mode_Read")
{
    SlotFS sfs;
    BlockDeviceMock<512,1000> bd;
    Config cfg;
    cfg.slot_count = 10;
    cfg.start_address = 0;
    cfg.end_address = 512*100;
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(rc == Result_Success);

    SlotFS::File file;
    rc = sfs_file_open(&sfs, 10, Mode_WriteCreate, &file);
    REQUIRE( Result_Success == rc);

    SECTION("open not found") {
        SlotFS::File file;
        rc = sfs_file_open(&sfs, 1, Mode_Read, &file);
        REQUIRE( Result_Error_FileNotFound == rc);
        REQUIRE(sfs.lock_counters[1] == 0);
    }
    SECTION("open out of range slot") {
        SlotFS::File file;
        rc = sfs_file_open(&sfs, 100, Mode_Read, &file);
        REQUIRE( Result_Error_FileNotFound == rc);
    }

    SECTION("open ok") {
        SlotFS::File file;
        rc = sfs_file_open(&sfs, 10, Mode_Read, &file);
        REQUIRE( Result_Success == rc);
        REQUIRE(sfs.lock_counters[10] == 2);

        rc = sfs_file_close(&sfs, &file);
        REQUIRE( Result_Success == rc);
        REQUIRE(sfs.lock_counters[10] == 1);
    }
}

void fill_buffer_test_pattern(uint8_t* data, size_t size) {
    srand(124113324);
    uint8_t* cur = data;
    uint8_t* cur_end = data+size;
    while (cur < cur_end) {
        *cur = rand() % 255;
        cur++;
    }
}
TEST_CASE("page_block_address")
{
    SlotFS::File file;
    uint64_t block_address;
    uint64_t offset;

    SECTION("file 0 0") {
        file.block_page = new BlockPage(2*kBlockSize);
        file.md.start_address = 0;
        file.md.max_address   = file.md.start_address + kBlockSize*100;
        file.file_cursor = 0;
        page_block_address(file, &block_address, &offset);
        REQUIRE(block_address == 0);
        REQUIRE(offset == 0);
   }
    SECTION("file 1024 0") {
      file.block_page = new BlockPage(2*kBlockSize);
      file.md.start_address = 512;
      file.md.max_address   = file.md.start_address + kBlockSize*100;
      file.file_cursor = 0;
      page_block_address(file, &block_address, &offset);
      REQUIRE(block_address == 512);
      REQUIRE(offset == 0);
    }
    SECTION("file page 1024 x") {
         file.block_page = new BlockPage(2*kBlockSize);
         file.md.start_address = 512;
         file.md.max_address   = file.md.start_address + kBlockSize*100;
         file.file_cursor = 16;
         page_block_address(file, &block_address, &offset);
         REQUIRE(block_address == file.md.start_address);
         REQUIRE(offset == 16);

         file.file_cursor = 511;
         page_block_address(file, &block_address, &offset);
         REQUIRE(block_address == file.md.start_address);
         REQUIRE(offset == 511);

         file.file_cursor = 512;
         page_block_address(file, &block_address, &offset);
         REQUIRE(block_address == file.md.start_address);
         REQUIRE(offset == 512);

         file.file_cursor = 513;
         page_block_address(file, &block_address, &offset);
         REQUIRE(block_address == file.md.start_address);
         REQUIRE(offset == 513);

         // now we go to the next page
         file.file_cursor =  file.block_page->capacity() + 128;
         page_block_address(file, &block_address, &offset);
         REQUIRE(block_address == file.block_page->capacity()+file.md.start_address);
         REQUIRE(offset == 128);
       }
}
TEST_CASE("sfs seek")
{
    SlotFS::File file;

    SECTION("not opened ") {
        file.mode = Mode_Read;
        int rc = sfs_file_seek(nullptr, &file, 0);
        REQUIRE( Result_Error_Assert == rc);
    }
    SECTION("opened in write mode") {
        file.block_page = new BlockPage(2*kBlockSize);
        file.mode = Mode_WriteCreate;
        int rc = sfs_file_seek(nullptr, &file, 0);
        REQUIRE( Result_Error_Assert == rc);
    }
    SECTION("seek outside file size") {
        file.block_page = new BlockPage(2*kBlockSize);
        file.mode = Mode_Read;
        file.md.start_address = 1024;
        file.md.file_size     = 10;
        file.md.max_address   = file.md.start_address + 2*kBlockSize*100;
        file.file_cursor = 3;
        int rc = sfs_file_seek(nullptr, &file, 10);
        REQUIRE( Result_Error_Assert == rc);
        REQUIRE( 3 == file.file_cursor);
        rc = sfs_file_seek(nullptr, &file, 12);
        REQUIRE( Result_Error_Assert == rc);
        REQUIRE( 3 == file.file_cursor);
    }

    SECTION("seek ") {
        file.block_page = new BlockPage(2*kBlockSize);
        file.block_page->offset = 10;
        file.mode = Mode_Read;
        file.md.start_address = 1024;
        file.md.file_size     = 513;
        file.md.max_address   = file.md.start_address + 2*kBlockSize*100;
        file.file_cursor = 0;
        int rc = sfs_file_seek(nullptr, &file, 10);
        REQUIRE( Result_Success == rc);
        REQUIRE( 10 == file.file_cursor);
        REQUIRE( 0 == file.block_page->offset);

        rc = sfs_file_seek(nullptr, &file, 512);
        REQUIRE( Result_Success == rc);
        REQUIRE( 512 == file.file_cursor);

        rc = sfs_file_seek(nullptr, &file, 513);
        REQUIRE( Result_Error_Assert == rc);
    }

}


TEST_CASE("sfs write")
{
    SlotFS sfs;
    BlockDeviceMock<512,1000> bd;
    Config cfg;
    cfg.slot_count = 1;
    cfg.start_address = 0;
    cfg.end_address = 512*(3+2); // file size will be 2 blocks ( 3 reserved for file system)
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(rc == Result_Success);

    SlotFS::File file;
    rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file);
    REQUIRE( Result_Success == rc);

    SECTION("write beyond max file size") {
        uint8_t buf[kBlockSize*3];
        fill_buffer_test_pattern(buf, sizeof(buf));
        size_t written = 0;
        rc = sfs_file_write(&sfs, &file, buf, 1023, &written);
        REQUIRE( Result_Success == rc);
        rc = sfs_file_write(&sfs, &file, buf+1023, 513, &written);
        REQUIRE( Result_Error_Write == rc);
    }

    SECTION("write page size") {
        uint8_t buf[kBlockSize*2];
        uint8_t check_buf[kBlockSize*2];
        fill_buffer_test_pattern(buf, sizeof(buf));
        size_t written = 0;
        rc = sfs_file_write(&sfs, &file, buf, kBlockSize*2, &written);
        REQUIRE( Result_Success == rc);
        sfs_file_flush(&sfs, &file);
        REQUIRE( kBlockSize*2 == written);
        sfs.block_device->read(file.md.start_address, check_buf, kBlockSize*2);
        REQUIRE( 0 == memcmp(check_buf, buf, kBlockSize*2));
        REQUIRE( Result_Success == rc);
    }

    SECTION("write partial block") {
        uint8_t buf[kBlockSize];
        uint8_t check_buf[kBlockSize];
        fill_buffer_test_pattern(buf, sizeof(buf));
        size_t written = 0;
        rc = sfs_file_write(&sfs, &file, buf, 100, &written);
        REQUIRE( Result_Success == rc);
        REQUIRE( 100 == written);
        rc = sfs_file_write(&sfs, &file, buf+100, 412, &written);
        REQUIRE( Result_Success == rc);
        REQUIRE( 412 == written);

        sfs.block_device->read(file.md.start_address, check_buf, kBlockSize);
        REQUIRE( 0 != memcmp(check_buf, buf, kBlockSize));

        sfs_file_flush(&sfs, &file);

        sfs.block_device->read(file.md.start_address, check_buf, kBlockSize);
        REQUIRE( 0 == memcmp(check_buf, buf, kBlockSize));
    }
}

TEST_CASE("sfs write large")
{
    SlotFS sfs;
     BlockDeviceMock<512,1000> bd;
     Config cfg;
     cfg.slot_count = 1;
     cfg.start_address = 0;
     cfg.end_address = 512*1000; // file size will be 2 blocks ( 3 reserved for file system)
     int rc = sfs_init(&bd, cfg, &sfs);
     REQUIRE(rc == Result_Success);

     SlotFS::File file;
     rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file);
     REQUIRE( Result_Success == rc);

    SECTION("write partial block random") {
       uint8_t buf[kBlockSize*5];
       uint8_t check_buf[kBlockSize*5];
       fill_buffer_test_pattern(buf, sizeof(buf));

       size_t remaining = sizeof(buf);
       srand(1001);
       uint8_t* data = buf;
       uint8_t* data_end = buf + sizeof(buf);
       while (remaining > 0) {
          size_t to_write = std::min<size_t>(rand() % 100,remaining);
          size_t written = 0;
          rc = sfs_file_write(&sfs, &file, data, to_write, &written);
          sfs_file_flush(&sfs, &file);
          data += written;
          REQUIRE( Result_Success == rc);
          REQUIRE( to_write == written);
         // printf("written %d bytes\n",to_write);
          remaining-=to_write;
       }
       sfs_file_flush(&sfs, &file);
       sfs.block_device->read(file.md.start_address, check_buf, sizeof(check_buf));
       REQUIRE( 0 == memcmp(check_buf, buf, sizeof(check_buf)));
    }
}

TEST_CASE("sfs read")
{
    SlotFS sfs;
    BlockDeviceMock<512,1000> bd;
    Config cfg;
    cfg.slot_count = 1;
    cfg.start_address = 0;
    cfg.end_address = 512*100;
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(rc == Result_Success);

    SlotFS::File file;
    rc = sfs_file_open(&sfs, 0, Mode_WriteCreate, &file);
    uint8_t buf[kBlockSize*5];
    fill_buffer_test_pattern(buf, sizeof(buf));
    size_t written = 0;
    rc = sfs_file_write(&sfs, &file, buf, sizeof(buf), &written);
    rc = sfs_file_close(&sfs, &file);
    REQUIRE( Result_Success == rc);

    SECTION("read whole block") {
        uint8_t check_buf[kBlockSize];
        rc = sfs_file_open(&sfs, 0, Mode_Read, &file);
        REQUIRE( Result_Success == rc);
        size_t read;
        rc = sfs_file_read(&sfs, &file, check_buf, kBlockSize, &read);
        REQUIRE( Result_Success == rc);
        REQUIRE( kBlockSize == read);
        REQUIRE( 0 == memcmp(check_buf, buf, kBlockSize));
    }

    SECTION("read partial block") {
       uint8_t check_buf[kBlockSize];
       rc = sfs_file_open(&sfs, 0, Mode_Read, &file);
       REQUIRE( Result_Success == rc);
       size_t read;
       rc = sfs_file_read(&sfs, &file, check_buf, 64, &read);
       REQUIRE( Result_Success == rc);
       REQUIRE( 64 == read);

       rc = sfs_file_read(&sfs, &file, check_buf+64, 512-64, &read);
       REQUIRE( Result_Success == rc);
       REQUIRE( 512-64 == read);
       REQUIRE( 0 == memcmp(check_buf, buf, kBlockSize));
   }

    SECTION("read partial block larger than page") {
       uint8_t check_buf[kBlockSize*5];
       rc = sfs_file_open(&sfs, 0, Mode_Read, &file);
       REQUIRE( Result_Success == rc);
       size_t read;
       rc = sfs_file_read(&sfs, &file, check_buf, 1536, &read);
       REQUIRE( Result_Success == rc);
       REQUIRE( 1536 == read);

       rc = sfs_file_read(&sfs, &file, check_buf+1536, kBlockSize*5-1536, &read);
       REQUIRE( Result_Success == rc);
       REQUIRE( kBlockSize*5-1536 == read);
       REQUIRE( 0 == memcmp(check_buf, buf, sizeof(check_buf)));
   }

    SECTION("read file eof") {
       uint8_t check_buf[kBlockSize*6];
       rc = sfs_file_open(&sfs, 0, Mode_Read, &file);
       REQUIRE( Result_Success == rc);
       size_t read;
       rc = sfs_file_read(&sfs, &file, check_buf, kBlockSize*4, &read);
       REQUIRE( Result_Success == rc);
       REQUIRE( kBlockSize*4 == read);
       rc = sfs_file_read(&sfs, &file, check_buf+kBlockSize*4, kBlockSize*2, &read);
       REQUIRE( Result_Success_Eof == rc);
       REQUIRE( kBlockSize == read);
       REQUIRE( 0 == memcmp(check_buf, buf, kBlockSize*5));

       rc = sfs_file_read(&sfs, &file, check_buf+kBlockSize*4, kBlockSize*2, &read);
       REQUIRE( Result_Success_Eof == rc);
       REQUIRE( 0 == read);
   }

    SECTION("read with file seek") {
       uint8_t check_buf[kBlockSize*5];
       rc = sfs_file_open(&sfs, 0, Mode_Read, &file);
       REQUIRE( Result_Success == rc);
       size_t read;
       for (int i=0; i < 5; i++) {
           rc = sfs_file_seek(&sfs,&file,i*kBlockSize);
           REQUIRE( Result_Success == rc);
           rc = sfs_file_read(&sfs, &file, check_buf+(i*kBlockSize), kBlockSize, &read);
           REQUIRE( Result_Success == rc);
           REQUIRE( kBlockSize == read);
       }
       REQUIRE( 0 == memcmp(check_buf, buf, kBlockSize*5));

   }
}

TEST_CASE("sfs read platform")
{

    BlockDeviceMock<512,10000> bd;
    SlotFS sfs;
    Config cfg;
    cfg.slot_count = 10;
    cfg.start_address = 0;
    cfg.end_address = 512*10000;
    int rc = sfs_init(&bd, cfg, &sfs);
    REQUIRE(Result_Success ==  rc);

    SlotFS::File file;
    rc = sfs_file_open(&sfs, 11, Mode_WriteCreate, &file);
    REQUIRE(Result_Error_FileNotFound == rc);

    rc = sfs_file_open(&sfs, 1, Mode_WriteCreate, &file);
    REQUIRE(Result_Success ==  rc);


    for (int i=0; i < 100; i++) {
        uint8_t buf[64];
        memset(buf,i,sizeof(buf));
        size_t written = 0;
        rc = sfs_file_write(&sfs,&file,buf,sizeof(buf), &written);
        REQUIRE(Result_Success ==  rc);
        REQUIRE(64 ==  written);
    }
    rc= sfs_file_close(&sfs,&file);
    REQUIRE(Result_Success ==  rc);
    rc = sfs_file_open(&sfs, 1, Mode_Read, &file);
    REQUIRE(Result_Success ==  rc);
    for (int i=0; i < 100; i++) {
        uint8_t buf[64];
        memset(buf,i,sizeof(buf));
        size_t read = 0;
        rc = sfs_file_read(&sfs,&file,buf,sizeof(buf), &read);

        REQUIRE(Result_Success ==  rc);
        REQUIRE(64 ==   read);
        REQUIRE(i ==  buf[0]);
    }
    sfs_file_close(&sfs,&file);

    rc = sfs_deinit(&sfs);
    REQUIRE(Result_Success ==  rc);
}
