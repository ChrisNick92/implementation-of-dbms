#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 10 // you can change it if you want
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
    int fd1;
    HP_info hpInfo, *p;
    int check, i;
    BF_Block *block;
    BF_Block_Init(&block);
    BF_ErrorCode code;
    void *data;
    Record record;
    HP_block_info* block_info;

    CALL_OR_DIE(BF_Init(LRU));

    check = HP_CreateFile("test.db");
    printf("Check 1: %d\n", check);

    if ((p = HP_OpenFile("test.db", &fd1)) == NULL){
        printf("Error\n");
        return -1;
    }

    printf("%d\n", p->total_block_recs);
    printf("%d \n", p->recorded_blocks);
    printf("Ok\n");

    for (i = 0; i < p->total_block_recs+8; i++){
        record = randomRecord();
        code = HP_InsertEntry(fd1, p, record);
        
        if (code != BF_OK){
          printf("Error %d!\n", i);
          return -1;
        }
    }

    int blocks_num;
    CALL_OR_DIE(BF_GetBlockCounter(fd1, &blocks_num));

    printf("Blocks num: %d\n", blocks_num);
    Record *rec;

    CALL_OR_DIE(BF_GetBlock(fd1, 0, block));
    HP_info* info = HP_AccessInfo(block);
    printf("Total blocks recorded %d \n",info->recorded_blocks);
    printf("\n\nContents of Blocks\n\n");

    for (int i = 1; i < blocks_num; ++i) { // Skip Block zero - Contains metadata
        printf("Contents of Block %d\n", i);
        CALL_OR_DIE(BF_GetBlock(fd1, i, block));
        block_info = HP_AccessBlockInfo(block);
        
        for (int j=1; j < block_info->number_of_records + 1; j++){
            
            rec = HP_GetBlockRecord(block, j);
            if (rec)
              printRecord(*rec);
        }
        CALL_OR_DIE(BF_UnpinBlock(block));
    }
    printf("RUN PrintAllEntries\n");
    int id = rand() % RECORDS_NUM;
    printf("\nSearching for: %d \n",id);
    int count = HP_GetAllEntries(fd1, p, id);
    printf("Found %d \n",count);
    BF_Block_Destroy(&block);
    check = HP_CloseFile(fd1, p);
    printf("Check 2: %d\n", check);
    CALL_OR_DIE(BF_Close());

    return 0;
}