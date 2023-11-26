#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
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

    if ((code = BF_CreateFile(fileName)) != BF_OK)
        return -1;
    // Create File
    if ((code = BF_OpenFile(fileName, &fd)) != BF_OK) 
        return -1;
    BF_Block_Init(&block);
    // Initialize header info
   
    p->total_block_recs = (BF_BLOCK_SIZE - sizeof(block_info)) / sizeof(rec);
    p->recorded_blocks = 0;
    // Write on Block
    if ((code = BF_AllocateBlock(fd, block)) != BF_OK){
        BF_PrintError(code);
        return -1;
    }
    // Write header on block

    data = BF_Block_GetData(block);
    p = data;
    p[0] = hpInfo;
    BF_Block_SetDirty(block);

    if ((code = BF_UnpinBlock(block)) != BF_OK){
      BF_PrintError(code);
      return -1;
    }

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

    if ((code = BF_GetBlock(file_desc, 0, block)) != BF_OK){
        BF_PrintError(code);
        return -1;
    }

    if ((code = BF_UnpinBlock(block)) != BF_OK){
      BF_PrintError(code);
      return -1;
    }

    BF_Block_Destroy(&block);

    if ((code = BF_CloseFile(file_desc)) != BF_OK){
      BF_PrintError(code);
      return -1;
    }

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

    if ((code = BF_GetBlock(file_desc, current_block, block)) != BF_OK){
        BF_PrintError(code);
        return -1;
    }
    rec = HP_GetNextBlockRecord(block, hp_info);

    if (rec) {                              // If rec Not Null, then it has space to store record
        rec[0] = record;
        block_info = HP_AccessBlockInfo(block);
        ++block_info->number_of_records;
        BF_Block_SetDirty(block);

        if ((code = BF_UnpinBlock(block)) != BF_OK){
            BF_PrintError(code);
            return -1;
        }
        else {
            return 0;
        }
    }
    else { //Current block is full, need to write a new one
        if(current_block != 0){
          if ((code = BF_UnpinBlock(block)) != BF_OK){ //Unpin current block
              BF_PrintError(code);
              return -1;
          }
        }
        if ((code = BF_AllocateBlock(file_desc, block)) != BF_OK){ // Initialize a new block
            BF_PrintError(code);
            return -1;
        }
        // Create Block Info and write record
        block_info = HP_AccessBlockInfo(block);
        block_info->number_of_records = 0;
        rec = HP_GetNextBlockRecord(block, hp_info);
        rec[0] = record;
        ++block_info->number_of_records; // Inceremnt record number for this block
        BF_Block_SetDirty(block);
        ++hp_info->recorded_blocks;

        if ((code = BF_UnpinBlock(block)) != BF_OK){ //Unpin Block
            BF_PrintError(code);
            return -1;
        }
    }
    if ((code = BF_GetBlock(file_desc, 0, block)) != BF_OK){
        BF_PrintError(code);
        return -1;
    }
    BF_Block_SetDirty(block);
    if ((code = BF_UnpinBlock(block)) != BF_OK){
      BF_PrintError(code);
      return -1;
    }
    return 0;
}

int HP_GetAllEntries(int file_desc, HP_info* hp_info, int value){
    int blocks_num,count_blocks = 0;
    BF_ErrorCode code;
    BF_Block *block;
    BF_Block_Init(&block);
    HP_block_info* block_info;
    Record* rec;
    if((code = BF_GetBlockCounter(file_desc, &blocks_num)) != BF_OK) return -1;

    for (int i = 1; i < blocks_num; ++i) {                  // Skip Block zero - Contains metadata
       
        if ((code = BF_GetBlock(file_desc, i, block)) != BF_OK){
            BF_PrintError(code);
            return -1;
        }
        block_info = HP_AccessBlockInfo(block);
        
        for (int j=1; j < block_info->number_of_records + 1; j++){
            
            rec = HP_GetBlockRecord(block, j);
            if (rec->id == value){
                count_blocks++;
                printRecord(*rec);
            }

        }
        if ((code = BF_UnpinBlock(block)) != BF_OK){
            BF_PrintError(code);
            return -1;
        }
    }   
    return count_blocks;
}