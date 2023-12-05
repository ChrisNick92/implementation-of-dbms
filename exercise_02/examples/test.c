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

const char* names[] = {
  "Yannis",
  "Christofos",
  "Sofia",
  "Marianna",
  "Vagelis",
  "Maria",
  "Iosif",
  "Dionisis",
  "Konstantina",
  "Theofilos",
  "Giorgos",
  "Dimitris"
};

const char* surnames[] = {
  "Ioannidis",
  "Svingos",
  "Karvounari",
  "Rezkalla",
  "Nikolopoulos",
  "Berreta",
  "Koronis",
  "Gaitanis",
  "Oikonomou",
  "Mailis",
  "Michas",
  "Halatsis"
};

const char* cities[] = {
  "Athens",
  "San Francisco",
  "Los Angeles",
  "Amsterdam",
  "London",
  "New York",
  "Tokyo",
  "Hong Kong",
  "Munich",
  "Miami"
};


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
    printf("%d, %d, %d, %d\n", p->global_depth, p->max_nodes_per_block, p->num_node_blocks, p->num_blocks);

    LList *root;
    if ((root = HT_HashTable_toList(fd1)) == NULL){
        printf("Failed to conver to list!\n");
        return -1;
    }
    HT_PrintHashTable(root);

    //Insert
    Record record;
    srand(12569874);
    int r;
    record.id = 123;
    r = rand() % 12;
    memcpy(record.name, names[r], strlen(names[r]) + 1);
    r = rand() % 12;
    memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
    r = rand() % 10;
    memcpy(record.city, cities[r], strlen(cities[r]) + 1);

    if ((code1 = HT_InsertEntry(fd1, root, record)) != HT_OK)
        return -1;
    
    if ((code1 = BF_GetBlock(fd1, 2, block)) != BF_OK)
        return -1;
    data = BF_Block_GetData(block);
    HT_BucketInfo *pBinfo;
    pBinfo = data;
    printf("%d, %d\n", pBinfo->local_depth, pBinfo->num_records);
    data = pBinfo +1;
    Record *prec;
    prec=data;
    printf("%s, %s, %d\n", prec->city, prec->name, prec->id);

    if ((code1 = BF_UnpinBlock(block)) != BF_OK)
        return -1;
    HT_PrintHashTable(root);
    if ((code1 = HT_CloseFile(fd1)) != HT_OK){
        printf("Failed to close\n");
        return -1;
    }

    BF_Block_Destroy(&block);
    CALL_OR_DIE(BF_Close());

    // LList *root;
    // root = HT_LList_Init();
    // (root->next)->block_num = 123;
    // HT_PrintHashTable(root);
    // HT_ExpandHashTable(root);
    // HT_UpdateHashTable(root, 1);
    // printf("\n");
    // HT_PrintHashTable(root);
    // printf("ok!!\n");
    // LList *s;
    // s = HT_GetHashTableBlockNum(root, 3);
    // printf("%d\n", s->block_num);
    // HT_SplitHashTable(root, 0, 22);
    // HT_PrintHashTable(root);

    return 0;
}