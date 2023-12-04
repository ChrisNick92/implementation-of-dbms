#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bf.h"
#include "hash_file.h"

#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }


int main(void){
    BF_Block *block;
    BF_Block_Init(&block);
    int fd1, *node;
    HT_FileInfo htInfo, *p=&htInfo;
    BF_ErrorCode code;
    HT_ErrorCode code1;
    void *data;
    Record rec;

    CALL_OR_DIE(BF_Init(LRU));

    if ((code1 = HT_CreateIndex("test.db", 1)) != HT_OK){
        printf("Failed creating.\n");
        return -1;
    }
    
    if ((p = HT_OpenIndex("test.db", &fd1)) == NULL){
        printf("Failed to open\n");
        return -1;
    }
    printf("%d, %d, %d\n", p->global_depth, p->max_nodes_per_block, p->num_node_blocks);

    LList *root;
    if ((root = HT_HashTable_toList(fd1)) == NULL){
        printf("Failed to conver to list!\n");
        return -1;
    }
    HT_PrintHashTable(root);

    if ((code1 = HT_CloseFile(fd1)) != HT_OK){
        printf("Failed to close\n");
        return -1;
    }

    BF_Block_Destroy(&block);
    CALL_OR_DIE(BF_Close());

    printf("ok!!\n");

    return 0;
}