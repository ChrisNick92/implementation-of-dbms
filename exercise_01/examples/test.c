#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 10          // you can change it if you want
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
    int fd1 ,blocks_num;
    HP_info hpInfo, *p;
    int check, i;
    BF_Block *block;
    BF_Block_Init(&block);
    BF_ErrorCode code;
    void *data;
    Record record;
    Record *rec;
    HP_block_info* block_info;

    CALL_OR_DIE(BF_Init(LRU));
    //Create file, if it does not exists
    if ((code = HP_CreateFile("test.db"))!= BF_OK){
      BF_PrintError(code);
    }

    //Open file
    if ((p = HP_OpenFile("test.db", &fd1)) == NULL){
        printf("Error\n");
        return -1;
    }
    //Insert records
    for (i = 0; i < p->total_block_recs+10; i++){
        record = randomRecord();
        CALL_OR_DIE(HP_InsertEntry(fd1, p, record));
    }

    CALL_OR_DIE(BF_GetBlockCounter(fd1, &blocks_num));

    printf("\n\nContents of Blocks\n\n");
    printf("Metadata of block 0 \n");
    printf("Capacity of each block : %d records\n", p->total_block_recs);
    printf("Number of blocks with records : %d\n\n", p->recorded_blocks);

    //Print the records of each block
    for (int i = 1; i < blocks_num; ++i) {            // Skip Block zero - Contains metadata
        printf("Contents of Block %d\n", i);
        CALL_OR_DIE(BF_GetBlock(fd1, i, block));
        block_info = HP_AccessBlockInfo(block);
        
        for (int j=1; j < block_info->number_of_records; j++){
            rec = HP_GetBlockRecord(block, j);
            if (rec)
              printRecord(*rec);
        }
        printf("\n");
        CALL_OR_DIE(BF_UnpinBlock(block));
    }

    //Search for id
    int id = rand() % RECORDS_NUM;
    printf("\nSearching for: %d \n",id);
    int count = HP_GetAllEntries(fd1, p, id);
    printf("Blocks %d \n",count);

    BF_Block_Destroy(&block);
    CALL_OR_DIE(HP_CloseFile(fd1, p));
    CALL_OR_DIE(BF_Close());

    return 0;
}