#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bf.h"
#include "hash_file.h"

#define RECORDS_NUM 1000
#define MAX_OPEN_FILES 20

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

int FILES[MAX_OPEN_FILES];

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
        printf("Already exists!\n");
    }
    
    if ((p = HT_OpenIndex("test.db", &fd1)) == NULL){
        printf("Failed to open\n");
        return -1;
    }
    printf("%d, %d, %d, %d\n", p->global_depth, p->max_records_per_bucket, p->num_node_blocks, p->num_blocks);

    LList *root;
    if ((root = HT_HashTable_toList(fd1)) == NULL){
        printf("Failed to conver to list!\n");
        return -1;
    }

    //Insert
    Record record;
    time_t t;

    srand(1231344);

    int r;
    for (int id = 0; id < RECORDS_NUM; id++){
        record.id = id;
        r = rand() % 12;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        r = rand() % 12;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        r = rand() % 10;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);
        if ((code1 = HT_InsertEntry(fd1, root, record)) != HT_OK)
            return -1;
    }
    
    // COMMENTS FOR SANITY CHECK

    // // Print Records for the first two blocks
    // if ((code1 = BF_GetBlock(fd1, 2, block)) != BF_OK)
    //     return -1;
    // data = BF_Block_GetData(block);
    // HT_BucketInfo *BInfo;
    // BInfo = data;
    // printf("Block 2: Records: %d Depth: %d\n", BInfo->num_records, BInfo->local_depth);
    // data = BInfo + 1;
    // Record *prec = data;
    // for (int i = 0; i < BInfo->num_records; i++)
    //     printf("id: %d\n", prec[i].id);
    // if ((code1 = BF_UnpinBlock(block)) != BF_OK)
    //     return -1;
    
    // if ((code1 = BF_GetBlock(fd1, 3, block)) != BF_OK)
    //     return -1;
    // data = BF_Block_GetData(block);
    // BInfo = data;
    // data = BInfo + 1;
    // prec = data;
    // printf("Block 3: Records: %d Depth: %d\n", BInfo->num_records, BInfo->local_depth);
    // for (int i = 0; i < BInfo->num_records; i++)
    //     printf("id: %d\n", prec[i].id);
    
    // if ((code1 = BF_UnpinBlock(block)) != BF_OK)
    //     return -1;

    // if ((code1 = BF_GetBlock(fd1, 4, block)) != BF_OK)
    //     return -1;
    // data = BF_Block_GetData(block);
    // BInfo = data;
    // data = BInfo + 1;
    // prec = data;
    // printf("Block 4: Records: %d Depth: %d\n", BInfo->num_records, BInfo->local_depth);
    // for (int i = 0; i < BInfo->num_records; i++)
    //     printf("id: %d\n", prec[i].id);
    // if ((code1 = BF_UnpinBlock(block)) != BF_OK)
    //     return -1;
    
    // Record p_recs[8];
    // memcpy(p_recs, data, 8 * sizeof(rec));
    // for (int i = 0; i < 8; i++)
    //     printf("%d ", p_recs[i].id);
    // printf("\n");

    // printf("Before List to HashTable\n");
    // HT_PrintHashTable(root);
    // HT_PrintAllEntries(fd1, root, NULL);
    // HashStatistics("test.db",root);

    // HT_List_to_HashTable(fd1, root);
    
    // root =  HT_HashTable_toList(fd1);
    // printf("After HashTable to List\n");
    // HT_PrintHashTable(root);

    // END COMMENTS FOR SANITY CHECK

    printf("RUN PrintAllEntries\n");
    int id = rand() % RECORDS_NUM;
    CALL_OR_DIE(HT_PrintAllEntries(fd1, root, &id));

    printf("RUN HashStatistics\n");
    HashStatistics("test.db", root);


    if ((code1 = HT_CloseFile(fd1, root)) != HT_OK){
        printf("Failed to close\n");
        return -1;
    }

    BF_Block_Destroy(&block);
    CALL_OR_DIE(BF_Close());
    
    return 0;
}