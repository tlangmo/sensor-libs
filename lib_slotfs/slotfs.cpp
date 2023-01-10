// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#include "slotfs.h"
#include "util_crc32.h"
#include <assert.h>
#include <lw_event_trace.h>
#include <cmath>
#include <cstdlib>

namespace motesque {

namespace slotfs {

void page_block_address(const SlotFS::File& file, uint64_t* block_address, uint64_t* offset ) {
    assert(file.md.start_address + file.file_cursor <= file.md.max_address);
    *block_address = file.md.start_address + (file.file_cursor/file.block_page->capacity())*file.block_page->capacity();
    *offset = file.file_cursor%file.block_page->capacity();
}


/*
 *  Calculates the crc32 of 508 bytes of data and appends it.
 *  This is used to detect corrupt meta data blocks
 */
static void add_block_crc(uint8_t* block_buffer, size_t size)
{
    uint32_t crc = motesque::crc32(block_buffer,size-sizeof(uint32_t));
    // write the crc at the end of the block
    *((uint32_t*)&block_buffer[size-sizeof(uint32_t)]) = crc;
}

/*
 *  Calculates the crc32 of 508 bytes of data checks it against the stored one (at the end).
 *  This is used to detect corrupt meta data blocks
 */
static bool check_block_crc(uint8_t* block_buffer, size_t size)
{
    uint32_t crc_have = motesque::crc32(block_buffer,size-sizeof(uint32_t));
    uint32_t crc_should = *((uint32_t*)&block_buffer[size-sizeof(uint32_t)]);
    // write the crc at the end of the block
    return crc_should == crc_have;
}

/*
 * Initialized the file system. If the config matches, it loads the fs, otherwise it formats it
 */
int sfs_init(BlockDevice* block_device, const Config& config, SlotFS* sfs)
{
    sfs->block_device = block_device;
    sfs->config = config;
    // check whether the device already has the same filesystem on it. If not format
    uint8_t block_buffer[kBlockSize+1];
    memset(block_buffer,0,sizeof(block_buffer));
    memset(sfs->lock_counters,0,sizeof(sfs->lock_counters));
    if (0 != block_device->read(config.start_address, block_buffer, kBlockSize)) {
        return Result_Error_Read;
    }
    Config* existing_config = (Config*)block_buffer;
    if (*existing_config != config) {
       return sfs_format(sfs);
    }
    return Result_Success;
}

/*
 * Writes a metadata block representing a certain slot. Since MetaDataBlocks come in pairs,
 * an index specifies which one to store
 */
static int write_meta_data_block(SlotFS* sfs, size_t slot, MetaDataBlock md, uint32_t idx) {
    uint8_t block_buffer[kBlockSize];
    if (idx > 2 ) {
        return Result_Error_Assert;
    }
    memset(block_buffer,0,sizeof(block_buffer));
    memcpy(block_buffer,&md,sizeof(md));
    add_block_crc(block_buffer, kBlockSize);

    uint64_t meta_block_address = sfs->config.start_address + kBlockSize*(slot*2+1+idx);
    if (0 != sfs->block_device->write(meta_block_address, block_buffer, kBlockSize)) {
        // this is fatal
        return Result_Error_Write;
    }
    return Result_Success;
}

/*
 * Reads a metadata block for a certain slot. Since MetaDataBlocks come in pairs,
 * an index specifies which one to read
 */
static int read_meta_data_block(SlotFS* sfs, size_t slot, MetaDataBlock* md, uint32_t idx) {
    uint8_t block_buffer[kBlockSize];
    memset(block_buffer,0,sizeof(block_buffer));
    uint64_t meta_block_address = sfs->config.start_address + kBlockSize*(slot*2+1+idx);
    if (0 != sfs->block_device->read(meta_block_address, block_buffer, kBlockSize)) {
        // this is fatal
        return Result_Error_Read;
    }

    if (check_block_crc(block_buffer,kBlockSize)) {
    //if (true) {
        memcpy(md,block_buffer,sizeof(MetaDataBlock));
        return Result_Success;
    }
    return Result_Error_CorruptData;
}

/*
 * Initialize a meta data block for a given slot
 */
static int init_meta_data_block(const Config& cfg, size_t slot,  MetaDataBlock* md) {
    uint64_t fs_total_size = (cfg.end_address - cfg.start_address);
    uint64_t fs_data_size = fs_total_size - kBlockSize * (1 + cfg.slot_count*2);
    uint64_t slot_size = ((fs_data_size / cfg.slot_count) / kBlockSize) * kBlockSize;
    uint64_t slot_start_address = cfg.start_address + kBlockSize * (1 + cfg.slot_count*2) + slot_size*slot;
   // assert(slot_start_address % kBlockSize == 0);
    memset(md,0,sizeof(MetaDataBlock));
    md->max_address = slot_start_address + slot_size;
    md->start_address = slot_start_address;
    return Result_Success;
}

/*
 * Format the filesystem
 */
int sfs_format(SlotFS* sfs)
{
    // write config block
    uint8_t block_buffer[kBlockSize];
    memset(block_buffer,0,sizeof(block_buffer));
    memcpy(block_buffer,&sfs->config, sizeof(sfs->config));
    if (0 != sfs->block_device->write(sfs->config.start_address, block_buffer, kBlockSize)) {
        return Result_Error_Write;
    }
    for (size_t s=0; s < sfs->config.slot_count; s++) {
        MetaDataBlock md;
        init_meta_data_block(sfs->config,s, &md);
        write_meta_data_block(sfs,s,md,0);
        write_meta_data_block(sfs,s,md,1);
    }
    return Result_Success;
}

int most_recent_meta_data_block(const MetaDataBlock& md_a, bool md_valid_a,
                                const MetaDataBlock& md_b, bool md_valid_b,
                                size_t* most_recent_idx) {
    if (md_valid_a && md_valid_b) {
        //  find most recent
        if (abs(md_a.revision-md_b.revision) > 1) {
            *most_recent_idx = md_a.revision < md_b.revision ?  0 :  1;
      }
      else {
          *most_recent_idx = md_a.revision < md_b.revision ?  1 :  0;
      }
    }
    else
        if (!md_valid_a && !md_valid_b) {
        return Result_Error_CorruptData;
    }
    else {
        *most_recent_idx = md_valid_a ? 0 : 1;
    }
    return Result_Success;
}

bool sfs_file_opened(const SlotFS::File* file)
{
    return file->block_page != nullptr;
}

bool sfs_file_exists(SlotFS* sfs, size_t slot )
{
    MetaDataBlock md[2];
    int rc_a = read_meta_data_block(sfs, slot, &md[0], 0);
    int rc_b = read_meta_data_block(sfs, slot, &md[1], 1);
    size_t most_recent_idx = 0;
    int rc = 0;
    if ((rc = most_recent_meta_data_block(md[0], rc_a == Result_Success,
                                          md[1], rc_b == Result_Success,
                                          &most_recent_idx)) != Result_Success) {
        return false;
    }
    // if the revision is '0', then there is no valid file at this slot
    return md[most_recent_idx].revision == 0 ? false : true;
}

/*
 * Open a file.
 */
int sfs_file_open(SlotFS* sfs, size_t slot, Mode mode, SlotFS::File* file)
{
    if (sfs_file_opened(file)) {
        // cannot open aleady opened file
        return Result_Error_Assert;
    }
    if (slot > sfs->config.slot_count) {
      return Result_Error_FileNotFound;
    }
    memset(file, 0,sizeof(SlotFS::File));

    MetaDataBlock md[2];
    if (mode == Mode_Read) {
        int rc_a = read_meta_data_block(sfs, slot, &md[0], 0);
        int rc_b = read_meta_data_block(sfs, slot, &md[1], 1);
        size_t most_recent_idx = 0;
        int rc = 0;
        if ((rc = most_recent_meta_data_block(md[0], rc_a == Result_Success,
                                              md[1], rc_b == Result_Success,
                                              &most_recent_idx)) != Result_Success) {
            return rc;
        }
        file->md = md[most_recent_idx];
        if (file->md.revision == 0) {
            return Result_Error_FileNotFound;
        }

    }
    else if (mode == Mode_WriteCreate && sfs->lock_counters[slot] == 0) {
        int rc = init_meta_data_block(sfs->config, slot, &file->md);
        if (Result_Success != rc) {
            return rc;
        }
        // new file start with revision 1
        file->md.revision = 1;
        for (int i=0;i < 2; i++) {
            rc = write_meta_data_block(sfs, slot, file->md, i);
            if (rc != Result_Success) {
                return rc;
            }
        }
    }
    else {
        // invalid mode or file already opened
        return Result_Error_Assert;
    }
    file->slot = slot;
    file->mode = mode;
    file->file_cursor = 0;
    //file->block_page = new BlockPage(mode == Mode_Read ? kBlockSize : kBlockSize*8);
    file->block_page = new BlockPage(kBlockSize*4);
    // keep track of opened files
    sfs->lock_counters[file->slot]++;
    return Result_Success;

}

/*
 * Change the file pointer of an opened file.
 * Clears the block page, so avoid using it often to avoid slow IO
 */
int sfs_file_seek(SlotFS* sfs, SlotFS::File* file, uint64_t offset)
{
    if (!sfs_file_opened(file) || file->mode == Mode_WriteCreate) {
        return Result_Error_Assert;
    }
    if (offset >= file->md.file_size) {
        return Result_Error_Assert;
    }
    file->file_cursor = offset;
    // clear the cache
    file->block_page->clear();
    return Result_Success;
}
int sfs_file_allocate(SlotFS* sfs, SlotFS::File* file, uint64_t size)
{
    if (!sfs_file_opened(file) || file->mode != Mode_WriteCreate) {
        return Result_Error_Assert;
    }
    file->file_cursor = size;
    // clear the cache
    file->block_page->clear();
    return sfs_file_flush(sfs,file);
}


int sfs_file_write(SlotFS* sfs, SlotFS::File* file, const uint8_t* data, uint64_t data_size, size_t* bytes_written)
{
    if (!sfs_file_opened(file)) {
        return Result_Error_Assert;
    }
    const uint8_t* cur_data = data;
    const uint8_t* end_data = cur_data + data_size;
    *bytes_written = 0;
    while (cur_data < end_data) {
        size_t remaining_data = end_data-cur_data;
        uint64_t remaining_file_space = (file->md.max_address-file->md.start_address)-file->file_cursor;
        //printf("remaining_data %d\n",remaining_data);
        //printf("remaining_file_space %d\n",remaining_file_space);
        if (remaining_data > remaining_file_space) {
            return Result_Error_Write;
        }
        size_t can_write_to_page = std::min<uint64_t>(remaining_data, file->block_page->available());
        //printf("can_write_to_page %d\n",can_write_to_page);
        memcpy(file->block_page->data + file->block_page->offset, cur_data, can_write_to_page);
        file->block_page->offset += can_write_to_page;
       //printf("file->block_page->free() %d\n",file->block_page->free());
        if (file->block_page->available() == 0) {
            uint64_t block_address = 0;
            uint64_t offset = 0;
            page_block_address(*file, &block_address, &offset);
            TRACE_EVENT0("slotfs","block_device write");
            if (0 != sfs->block_device->write(block_address, file->block_page->data, file->block_page->capacity())) {
                // abort if not possible.
                // Note that some previous data might have been written already.
                return Result_Error_Write;
            }
            file->block_page->clear();
            sfs_file_flush(sfs, file);
        }
        //advance cursor after potential page write to write to the correct block..
        cur_data +=can_write_to_page;
        *bytes_written += can_write_to_page;
        file->file_cursor += can_write_to_page;
    }
    return Result_Success;
}

int sfs_file_flush(SlotFS* sfs, SlotFS::File* file)
{
    if (!sfs_file_opened(file) || file->mode != Mode_WriteCreate) {
        return Result_Error_Assert;
    }
    if (file->block_page->size() > 0) {
        uint64_t block_address = 0;
        uint64_t offset = 0;
        page_block_address(*file, &block_address, &offset);
        TRACE_EVENT0("slotfs","block_device write flush");
        if (0 != sfs->block_device->write(block_address, file->block_page->data, file->block_page->capacity())) {
            // abort if not possible.
            // Note that some previous data might have been written already.
            return Result_Error_Write;
        }
    }

    MetaDataBlock md[2];
    // persist the file size
    file->md.file_size = file->file_cursor;
    file->md.revision++;
    int rc_a = read_meta_data_block(sfs, file->slot, &md[0], 0);
    int rc_b = read_meta_data_block(sfs, file->slot, &md[1], 1);
    size_t most_recent_idx = 0;
    int rc = 0;
    if ((rc = most_recent_meta_data_block(md[0], rc_a == Result_Success,
                                          md[1], rc_b == Result_Success,
                                          &most_recent_idx)) != Result_Success) {
        return rc;
    }
    rc = write_meta_data_block(sfs,file->slot, file->md, (most_recent_idx + 1) % 2);
    return Result_Success;
}

/*
 * Read data from file. The size does not need to be block aligned, but block aligned read is more performant.
 * Returns Result_Success or Result_Error_Eof
 */
int sfs_file_read(SlotFS* sfs, SlotFS::File* file, uint8_t* read_data, uint64_t read_size, size_t* bytes_read)
{
    uint8_t* cur_data = read_data;
    uint8_t* end_data = cur_data + read_size;
    *bytes_read = 0;
    if (!sfs_file_opened(file) || file->mode != Mode_Read) {
        return Result_Error_Assert;
    }
    while (cur_data < end_data) {
        if (file->file_cursor >= file->md.file_size) {
            // we have reached the end
            return Result_Success_Eof;
        }
        // load page if necessary
        if (file->block_page->available() == 0 || file->block_page->size() == 0) {
            uint64_t block_address = 0;
            uint64_t offset = 0;
            page_block_address(*file, &block_address, &offset);
            TRACE_EVENT0("slotfs","block_device read");
            if (0 != sfs->block_device->read(block_address, file->block_page->data, file->block_page->capacity())) {
                // abort if not possible.
                // Note that some previous data might have been written already.
                return Result_Error_Read;
            }
            // set correct offset in page
            file->block_page->offset = offset;
        }
        // take care not to read more then the file end
        size_t remaining_to_read = std::min<size_t>(end_data-cur_data, file->md.file_size-file->file_cursor);
        size_t can_read_from_page = std::min<size_t>(file->block_page->capacity()-file->block_page->size(), remaining_to_read);
        memcpy(cur_data, file->block_page->data+file->block_page->offset, can_read_from_page);

        *bytes_read += can_read_from_page;
        cur_data += can_read_from_page;
        file->file_cursor += can_read_from_page;
        file->block_page->offset += can_read_from_page;
    }
    return Result_Success;
}

int sfs_file_close(SlotFS* sfs, SlotFS::File* file)
{
    // 1. flush
    // 2. reset
    if (!sfs_file_opened(file)) {
        return Result_Error_Assert;
    }
    if (file->mode == Mode_WriteCreate) {
        int rc = sfs_file_flush(sfs,file);
        if ( rc != Result_Success) {
            return rc;
        }
    }
    sfs->lock_counters[file->slot]--;
    delete file->block_page;
    memset(file, 0,sizeof(SlotFS::File));
    return Result_Success;
}

int sfs_deinit(SlotFS* sfs)
{
    sfs->block_device = nullptr;
    memset(&sfs->config, 0,sizeof(Config));
    return Result_Success;
}


int sfs_file_size(const SlotFS::File* file, size_t* size)
{
    if (!sfs_file_opened(file)) {
        return Result_Error_Assert;
    }
    *size =  file->md.file_size;
    return Result_Success;
}


}; // ends
}; // ends




