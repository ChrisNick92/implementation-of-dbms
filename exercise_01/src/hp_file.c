#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

int HP_CreateFile(char *fileName){

    int fd;
    BF_Block *block;
    HP_info hpInfo, *p = &hpInfo;
    HP_block_info block_info;
    BF_ErrorCode code;
    Record rec;
    void *data;
    int blocks_num;

    // Create File
    CALL_BF(BF_CreateFile(fileName));


    CALL_BF(BF_OpenFile(fileName, &fd));
    
    BF_Block_Init(&block);

    // Initialize header info
    p->total_block_recs = (BF_BLOCK_SIZE - sizeof(block_info)) / sizeof(rec);
    p->recorded_blocks = 0;

    // Write on Block
    CALL_BF(BF_AllocateBlock(fd, block));
   
    // Write header on block
    data = BF_Block_GetData(block);
    p = data;
    p[0] = hpInfo;
    BF_Block_SetDirty(block);

    BF_Block_Destroy(&block);
    BF_CloseFile(fd);
    return 0;
}

HP_info* HP_OpenFile(char *fileName, int *file_desc){
    void *data;
    BF_Block *block;
    BF_Block_Init(&block);
    BF_ErrorCode code;
    HP_info *hpInfo;

    if ((code = BF_OpenFile(fileName, file_desc)) != BF_OK){
        BF_PrintError(code);
        return NULL;
    }
    if ((code = BF_GetBlock(*file_desc, 0, block)) != BF_OK){
        BF_PrintError(code);
        return NULL;
    }
    hpInfo = HP_AccessInfo(block);
    return hpInfo;
}


int HP_CloseFile(int file_desc, HP_info* hp_info ){

    BF_Block *block;
    BF_Block_Init(&block);
    BF_ErrorCode code;

    CALL_BF(BF_GetBlock(file_desc, 0, block));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));

    BF_Block_Destroy(&block);

    CALL_BF(BF_CloseFile(file_desc));

    return 0;
}

HP_block_info* HP_AccessBlockInfo(BF_Block *block){
    void *data;
    HP_block_info* block_info;

    data = BF_Block_GetData(block);
    block_info = data;

    return block_info;
}

HP_info* HP_AccessInfo(BF_Block *block){
    void *data;
    HP_info* info;

    data = BF_Block_GetData(block);
    info = data;

    return info;
}

Record* HP_GetNextBlockRecord(BF_Block *block, HP_info *hpInfo){
    void *data;
    HP_block_info *block_info;
    Record *rec;
    int total_block_records;

    total_block_records = hpInfo->total_block_recs;
    block_info = HP_AccessBlockInfo(block);

    if (block_info->number_of_records >= total_block_records) // No more space
        return NULL;
    // Find Next Free space
    data = block_info+1;
    rec = data;
    rec += (block_info->number_of_records);
    
    return rec;
}

Record *HP_GetBlockRecord(BF_Block *block, int record_num){
    HP_block_info *block_info;
    Record *rec;
    void *data;

    block_info = HP_AccessBlockInfo(block);
    if (record_num > block_info->number_of_records || record_num <= 0) //Error check
        return NULL;
    data = block_info + 1;              //next free space
    rec = data;
    rec += (record_num -1);             

    return rec;
}

int HP_InsertEntry(int file_desc, HP_info* hp_info, Record record){
    BF_Block *block;
    BF_Block_Init(&block);
    int current_block;
    BF_ErrorCode code;
    Record* rec;
    HP_block_info *block_info;

    current_block = hp_info->recorded_blocks; // Current Block to Insert Record
    if(current_block != 0) {
      CALL_BF(BF_GetBlock(file_desc, current_block, block));
      rec = HP_GetNextBlockRecord(block, hp_info);
    }
    else {
      rec = NULL;
    }
    if (rec) {                              // If rec Not Null, then it has space to store record
        rec[0] = record;
        block_info = HP_AccessBlockInfo(block);
    }
    else {                                  //Current block is full, need to write a new one
        if(current_block != 0){
            CALL_BF(BF_UnpinBlock(block));          //Unpin current block
        }
        CALL_BF(BF_AllocateBlock(file_desc, block));    // Initialize a new block
        // Create Block Info and write record
        block_info = HP_AccessBlockInfo(block);
        block_info->number_of_records = 0;
        rec = HP_GetNextBlockRecord(block, hp_info);
        rec[0] = record;
        ++hp_info->recorded_blocks;
    }

    ++block_info->number_of_records; // Incerement record number for this block
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));

    return 0;
}


int HP_GetAllEntries(int file_desc, HP_info* hp_info, int value){
    int blocks_num,count_blocks = 0,found =0;
    BF_ErrorCode code;
    BF_Block *block;
    BF_Block_Init(&block);
    HP_block_info* block_info;
    Record* rec;
    if((code = BF_GetBlockCounter(file_desc, &blocks_num)) != BF_OK) return -1;

    for (int i = 1; i < blocks_num; ++i) {                  // Skip Block zero - Contains metadata
        CALL_BF(BF_GetBlock(file_desc, i, block));
        block_info = HP_AccessBlockInfo(block);
        count_blocks++;
        for (int j=1; j < block_info->number_of_records + 1; j++){
            
            rec = HP_GetBlockRecord(block, j);
            if (rec->id == value){
                found = 1;
                printRecord(*rec);
            }
        }
        CALL_BF(BF_UnpinBlock(block));
        
    }
    if(!found){
        return -1;
    }
    return count_blocks;
}