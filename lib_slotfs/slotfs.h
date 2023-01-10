// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#pragma once
#include <cstring>
#include <string>
#include <map>
#include <memory>
#include <malloc.h>

namespace motesque {

namespace slotfs {

/*
 * Compile time configuration
 */
enum {
    kBlockSize = 512,
    kMaxSlots  = 128
};

/*
 * Config for file system. Several filesystems can be places on disk
 */
struct Config
{
   size_t   slot_count;
   uint64_t start_address;
   uint64_t end_address;
   bool operator==(const Config& rhs) {
       return (slot_count == rhs.slot_count) && (start_address == rhs.start_address) && (end_address == rhs.end_address);
   }
   bool operator!=(const Config& rhs) {
      return !(*this == rhs);
  }
};

/*
 * The file open mode
 */
enum Mode {
    Mode_WriteCreate = 0,
    Mode_Read,
    Mode_Unknown
};

/*
 * Descriptive Return type
 */
enum Result {
    Result_Success = 0,
    Result_Success_Eof = 1,
    Result_Error_Read  = -1,
    Result_Error_Write = -2,
    Result_Error_CorruptData = -3,
    Result_Error_Assert = -4,
    Result_Error_FileNotFound = -5,
};

/*
 * File descriptor entry for file system. Each slot has two Meta Data Blocks to be robust against disk failures such as power loss.
 * The block with the most current revision is the active one
 */
struct MetaDataBlock
{
   uint64_t start_address;  // the start address of the file on disk
   uint64_t max_address;    // the possible max address of the file on disk
   uint64_t file_size;      // the current size of the file
   uint8_t  revision;       // the revision of this meta data
};

/*
 * A BlockPage holds the data of multiple blocks and abstract block boundary requirement of block devices.
 * Every file has one attached. The size needs to be a multiple of blocksize
 */
class BlockPage {
public:
    BlockPage(size_t size) : data_size(size){
        data = (uint8_t*)memalign(32, data_size); // make sure we align correctly for the DMA
        memset(data,0,data_size);
        offset = 0;
    }
    virtual ~BlockPage() {
        free(data);
    }

    size_t size() const {
        return offset;
    }

    size_t available() const {
        return data_size-offset;
    }

    size_t capacity() const {
        return data_size;
    }

    void clear() {
        offset = 0;
    }

    size_t data_size;
    uint8_t*  data;
    size_t    offset;

};



/*
 * Interface for a generic block device
 */
class BlockDevice {
public:
    virtual ~BlockDevice() {}
    virtual int write(uint64_t start_address, const uint8_t* data, uint64_t size) = 0;
    virtual int read(uint64_t start_address, uint8_t* data, uint64_t size) = 0;
};


/*
 * The Slot Filesystem has simplicity in mind. The disk is devided into N equal size slots. Those slots are addressed consecutively, so
 * no more bookkeeping than a simple offset is needed. All Metadata of a slot is saved in an own block at the start of the Filesystem and has
 * A/B redundancy. In case of a powerloss the previous version of the MetaBlock can be recovered.
 *
 * Reading and writing is buffered (hardcoded for now) to exploit efficiencies with multi-block operations. Although we found that this
 * depends highly on the underltying hardware (e.g SD card vs eMMC). So real world tweaking is advised
 */
struct SlotFS
{
    BlockDevice* block_device;
    Config config;
    int8_t lock_counters[kMaxSlots]; // every read increases the counter for a slot. Only for readers=0 can a file we opened for writing

    /*
     * A file of slot FS
     */
    struct File {
        File(): slot(0), md(), mode(Mode_Unknown), file_cursor(0), block_page(nullptr){

       }
       size_t size() const {
           return md.file_size;
       }
       size_t slot;
       MetaDataBlock md;
       Mode mode;
       uint64_t   file_cursor;      // the current position in the file
       BlockPage* block_page;    // the read/write buffer for efficient multi-block operations
    };

};

/*
 * Calculate the absolute block address of the current page from file position
 */
void page_block_address(const SlotFS::File& file, uint64_t* block_address, uint64_t* offset );

/*
 * compares two blocks of metadata.
 */
int most_recent_meta_data_block(const MetaDataBlock& md_a, bool md_valid_a,
                                const MetaDataBlock& md_b, bool md_valid_b,
                                size_t* most_recent_idx);

/*
 *  Initializes the FileSystem. Formats the disk if no or different FS is found
 */
int sfs_init(BlockDevice* block_device, const Config& config, SlotFS* sfs);

/*
 *  Deinitializes the FileSystem.
 */
int sfs_deinit(SlotFS* sfs);

/*
 * Format the FileSystem. It needs to be initialized first
 */
int sfs_format(SlotFS* sfs);

int sfs_file_open(SlotFS* sfs, size_t slot, Mode mode, SlotFS::File* file);
bool sfs_file_exists(SlotFS* sfs, size_t slot );
int sfs_file_close(SlotFS* sfs, SlotFS::File* file);
/*
 * Writes data to block device. The size does not need to be block aligned, but only full blocks are immediately written to device,
 * the rest stays in memory cache (until an explicit flush).
 * If a block device error occurs, the function returns with an error. Check the written
 * parameter how many bytes were already written successfully in that case.
 */
int sfs_file_write(SlotFS* sfs, SlotFS::File* file, const uint8_t* data, uint64_t data_size, size_t* bytes_written);
int sfs_file_read(SlotFS* sfs, SlotFS::File* file, uint8_t* data, uint64_t data_size,  size_t* bytes_read);

/*
 * flush the file. All pending data is written and the metadatablock updated
 */
int sfs_file_flush(SlotFS* sfs, SlotFS::File* file);
int sfs_file_seek(SlotFS* sfs, SlotFS::File* file, uint64_t pos);
int sfs_file_size(const SlotFS::File* file, size_t* size);
int sfs_file_allocate(SlotFS* sfs, SlotFS::File* file, uint64_t size);


}; //end ns slotfs
}; // end ns motesque
