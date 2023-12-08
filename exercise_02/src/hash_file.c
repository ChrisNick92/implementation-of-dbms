#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"


#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

HT_ErrorCode HT_Init() {
  
  return HT_OK;
}

/* HASH TABLE UTILS */

int HT_HashFunc(int k, int n_bits){
    int i,rev=0;
    char* bin = malloc((n_bits+1)*sizeof(char));
    for(i=0;i<n_bits;i++){
        int c = k%2;
        char ch = c + '0';
        bin[i] = ch;
        k >>= 1;
    }
    bin[i] = '\0';
    for(i=0; i<n_bits;i++){
        int k = bin[i] - '0';
        rev = 2 * rev + k;
    }
    free(bin);
    return rev;
}

LList * HT_LList_Init(){
    LList *root;
    if ((root = malloc(sizeof(*root))) == NULL)
        return NULL;
    else
        root->prev=NULL;
        root->next = malloc(sizeof(*root));
        (root->next)->next = NULL;
        (root->next)->prev = root;
        return root;
}

void HT_ExpandHashTable(LList *root){
    LList *p, *q;
    p = root;

    while (p->next != NULL){
        q = malloc(sizeof(*root));
        q->next = p->next;
        q->prev = p;
        p->next = q;
        p = q->next;
        p->prev = q;
    }
    q = malloc(sizeof(*root));
    p->next = q;
    q->prev = p;
    q->next = NULL;

}

void HT_UpdateHashTable(LList *root, int depth){
    LList *q = root->next;
    int index;

    while (q->next != NULL){
        q->block_num = (q->prev)->block_num;
        q = (q->next)->next;
    }
    q->block_num = (q->prev)->block_num;
}

int HT_HashTableLen(LList *root){
    LList *q = root;
    int n_nodes = 0;

    while(q->next != NULL){
        n_nodes++;
        q = q->next;
    }
    return n_nodes + 1;
}

void HT_PrintHashTable(LList *root){
    LList *q = root;
    int i=0;
    
    while(q->next != NULL){
        printf("Node: %3d | Block: %3d\n", i, q->block_num);
        q = q->next;
        i++;
    }
    printf("Node: %3d | Block: %3d\n", i, q->block_num);
}

void HT_SplitHashTable(LList *root, int target_block_num, int new_block_num){
    int total_neighbors = 0;
    LList *q = root, *starting_node;
    int i;

    // Find all neighbors
    while (q->next != NULL){
        if (q->block_num == target_block_num){
            // Starting node of the sequence found
            if (!total_neighbors){
                starting_node = q;
            }
            total_neighbors++;
        }
        else if (total_neighbors){
            break;
        }
        q = q->next;
    }
    // Check if last node is part of the sequence
    if (q->block_num == target_block_num)
        total_neighbors++;
    
    // Split sequence in half
    q = starting_node;
    for (i = 0; i < total_neighbors / 2; i++)
        q = q->next;
    for (i = total_neighbors / 2; i < total_neighbors; i++){
        q->block_num = new_block_num;
        q = q->next;
    }
}

LList *HT_HashTable_toList(int indexDesc){
    BF_Block *block;
    BF_Block_Init(&block);
    LList *root, *curr, *prev;
    void *data;
    HT_HashNodeInfo *HashNodeInfo;
    BF_ErrorCode code;
    HT_HashNodeInfo *NodeInfo;
    int next_block_num = 1, i, *node, j;
    HT_FileInfo *FileInfo;

    FileInfo = HT_GetFileInfo(indexDesc);

    // Loop over all node blocks
    for (j = 1; j <= FileInfo->num_node_blocks; j++){
        if ((code = BF_GetBlock(indexDesc, next_block_num, block)) != BF_OK)
            return NULL;
        data = BF_Block_GetData(block);
        NodeInfo = data;
        next_block_num = NodeInfo->next_block_num;
        data = NodeInfo + 1;
        node = data;
        for (i = 0; i < NodeInfo->num_block_nodes; i++){
            if (i == 0 && j == 1){ // On root
                root = malloc(sizeof(*root));
                root->block_num = node[i];
                root ->prev = NULL;
                prev = root;
            }
            else if ((i == NodeInfo->num_block_nodes -1) && (j == FileInfo->num_node_blocks))
            {
                curr = malloc(sizeof(*curr));
                curr->prev = prev;
                curr->block_num = node[i];
                prev->next = curr;
                curr->next = NULL;
            }
            else{
                curr = malloc(sizeof(*curr));
                curr->prev = prev;
                curr->block_num = node[i];
                prev->next = curr;
                prev = curr;

            }
            
        }
        if ((code = BF_UnpinBlock(block)) != BF_OK)
            return NULL;
    }
    return root;
}

LList * HT_GetHashTableBlockNum(LList *root, int n){
    LList *q = root;
    int i;

    for (i = 0; (q->next != NULL) && i < n; i++)
        q = q->next;
    if (i == n)
        return q;
    else
        return NULL; // Not found
}

HT_ErrorCode HT_List_to_HashTable(int indexDesc, LList *root){
    
    BF_Block *block;
    BF_Block_Init(&block);
    HT_FileInfo *FileInfo;
    HT_HashNodeInfo *NodeInfo, info;
    BF_ErrorCode bf_code;
    void *data;
    int *node, blocks_num;
    LList *current = root;
    int list_length, how_many_block_to_write, j, next_block_num, num_node_blocks;

    FileInfo = HT_GetFileInfo(indexDesc);
    list_length = HT_HashTableLen(root);
    next_block_num = FileInfo->hash_table_block_num;
    num_node_blocks = FileInfo->num_node_blocks;
    
    // Determine how to many blocks to write
    how_many_block_to_write = (list_length / FileInfo->max_nodes_per_block);
    if (list_length % FileInfo->max_nodes_per_block)
        how_many_block_to_write++;

    for (int i = 0; i < how_many_block_to_write; i++){
        if (i + 1 > num_node_blocks){ // Need to allocate new block
            FileInfo->num_node_blocks++;
            HT_GetBlockCounter(indexDesc, &blocks_num);
            NodeInfo->next_block_num = blocks_num;
            BF_Block_SetDirty(block);
            if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
                return HT_ERROR;
            if ((bf_code = BF_AllocateBlock(indexDesc, block)) != BF_OK)
                return HT_ERROR;
            FileInfo->num_blocks++;
            // Write info
            NodeInfo = &info;
            NodeInfo->next_block_num = -1;
            NodeInfo->num_block_nodes = 0;
            NodeInfo[0] = info;
        }
        else if (i == 0) { // We are in block_1
            if ((bf_code = BF_GetBlock(indexDesc, 1, block)) != BF_OK)
                return HT_ERROR;
            data = BF_Block_GetData(block);
            NodeInfo = data;
            NodeInfo->num_block_nodes = 0;
        } else {
            if ((bf_code = BF_GetBlock(indexDesc, next_block_num,  block)) != BF_OK)
                return HT_ERROR;
        }
        // Write records on block
        data = BF_Block_GetData(block);
        NodeInfo = data;
        data = NodeInfo + 1;
        node = data;
        j = 0;
        do {
            node[j] = current->block_num;
            j++;
            current = current->next;
            NodeInfo->num_block_nodes++;    
        } while ((j < FileInfo->max_nodes_per_block) && (current != NULL));

        next_block_num = NodeInfo->next_block_num;
        // Set dirty and Unpin and go to next block
        BF_Block_SetDirty(block);

        if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
            return HT_ERROR;
    }
    return HT_OK;
}

/* HASH TABLE UTILS END*/

HT_FileInfo * HT_GetFileInfo(int indexDesc){
    void *data;
    HT_FileInfo *p;
    BF_ErrorCode code;
    BF_Block *block;
    BF_Block_Init(&block);

    if ((code = BF_GetBlock(indexDesc, 0, block)) != BF_OK)
        return NULL;

    data = BF_Block_GetData(block);
    p = data;
    BF_Block_Destroy(&block);

    return p;
}

HT_ErrorCode HT_GetBlockCounter(const int indexDesc, int *blocks_num){
    HT_FileInfo *FileInfo;
    if ((FileInfo = HT_GetFileInfo(indexDesc)) == NULL)
        return HT_ERROR;
    *blocks_num = FileInfo->num_blocks;
    return HT_OK;
}


HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
    int fd;
    BF_Block *block;
    HT_FileInfo htInfo, *p = &htInfo;
    BF_ErrorCode code;
    void *data;
    Record rec;

    BF_Block_Init(&block);

    // Create file
    CALL_BF(BF_CreateFile(filename));
    // Open file
    CALL_BF(BF_OpenFile(filename, &fd));

    // Write info on block 0
    p->global_depth = depth;
    p->hash_table_block_num = 1;
    p->max_nodes_per_block = (BF_BLOCK_SIZE - sizeof(HT_HashNodeInfo)) / sizeof(int);
    p->max_records_per_bucket = (BF_BLOCK_SIZE - sizeof(HT_BucketInfo)) / sizeof(rec);
    p->num_node_blocks = 1;
    p->num_blocks = 2;

    if ((code = BF_AllocateBlock(fd, block)) != BF_OK)
        return HT_ERROR;
    data = BF_Block_GetData(block);
    p = data;
    p[0] = htInfo;

    BF_Block_SetDirty(block);

    if ((code = BF_UnpinBlock(block)) != BF_OK)
        return HT_ERROR;

    // Initialize first node block
    if ((code = BF_AllocateBlock(fd, block)) != BF_OK)
        return HT_ERROR;
    
    HT_HashNodeInfo HashNodeInfo, *q = &HashNodeInfo;
    int *node;

    q->next_block_num = -1;
    q->num_block_nodes = 2;

    data = BF_Block_GetData(block);
    q = data;
    q[0] = HashNodeInfo;

    data = q+1;
    node = data;
    node[0] = -1;
    node[1] = -1;

    BF_Block_SetDirty(block);

    if ((code = BF_UnpinBlock(block)) != BF_OK)
        return HT_ERROR;

    BF_CloseFile(fd);
    BF_Block_Destroy(&block);

    return HT_OK;
}

HT_FileInfo* HT_OpenIndex(const char *fileName, int *indexDesc){
    BF_ErrorCode code;
    HT_FileInfo *p;

    HT_FileInfo *FileInfo;

    if ((code = BF_OpenFile(fileName, indexDesc)) != BF_OK){
        printf("Open failed!\n");
        return NULL;
    }
    if ((p = HT_GetFileInfo(*indexDesc)) == NULL){
        printf("Here failed!\n");
        return NULL;
    }

    return p;
}

HT_ErrorCode HT_CloseFile(int indexDesc, LList *root) {
    BF_Block *block;
    BF_Block_Init(&block);
    BF_ErrorCode code;
    HT_ErrorCode ht_code;

    if ((ht_code = HT_List_to_HashTable(indexDesc, root)) != HT_OK)
        return HT_ERROR;

    if ((code = BF_GetBlock(indexDesc, 0, block)) != BF_OK)
        return HT_ERROR;
    BF_Block_SetDirty(block);
    if ((code = BF_UnpinBlock(block)) != BF_OK)
        return HT_ERROR;
    
    BF_Block_Destroy(&block);
    if ((code = BF_CloseFile(indexDesc)) != BF_OK)
        return HT_ERROR;

    return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, LList *HashTable, Record record) {
    BF_Block *block, *block_0, *new_block;
    BF_Block_Init(&block);
    BF_Block_Init(&block_0);
    BF_Block_Init(&new_block);
    HT_FileInfo *FileInfo;
    HT_ErrorCode ht_code;
    BF_ErrorCode bf_code;
    int hash_index;
    HT_BucketInfo BucketInfo, *pBInfo=&BucketInfo, *new_pBinfo=&BucketInfo;
    void *data;
    Record *prec;
    LList *s;
    int i, bucket_records;
    
    if ((bf_code = BF_GetBlock(indexDesc, 0, block_0)) != BF_OK)
        return HT_ERROR;
    data = BF_Block_GetData(block_0);
    FileInfo = data;
   
    // Determine bucket to insert the new record
    hash_index = HT_HashFunc(record.id,FileInfo->global_depth);
    // Find block num and read
    s = HT_GetHashTableBlockNum(HashTable, hash_index);

    if (s->block_num == -1){ // No bucket is initialized
        if ((bf_code = BF_AllocateBlock(indexDesc, block)) != BF_OK)
            return HT_ERROR;
        // Write bucket info first
        data = BF_Block_GetData(block);
        pBInfo->local_depth = 1;
        pBInfo->num_records = 1;
        pBInfo = data;
        pBInfo[0] = BucketInfo;
        // Write record
        data = pBInfo + 1;
        prec = data;
        prec[0] = record;

        // Update FileInfo
        (FileInfo->num_blocks)++;
        // Update HashTable
        s->block_num = FileInfo->num_blocks -1;
        // Set to dirty and Unpin
        BF_Block_SetDirty(block);
        if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
            return HT_ERROR;
        
        BF_Block_SetDirty(block_0);
    }
    else { 
        // Get Block
        if ((bf_code = BF_GetBlock(indexDesc, s->block_num, block)) != BF_OK)
            return HT_ERROR;
        data = BF_Block_GetData(block);
        pBInfo = data;
        // Check if there is free space
        if (pBInfo->num_records < FileInfo->max_records_per_bucket){ // There is free space
            // Find place to insert record
            data = pBInfo + 1;
            prec = data;
            prec[pBInfo->num_records] = record;
            // Update Bucket Info
            (pBInfo->num_records)++;
            // Set dirty and Unpin
            BF_Block_SetDirty(block);
            if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
                return HT_ERROR;
        }
        else { // No free space
            // Case 1: local_depth < global_depth -> No expansion

            if (pBInfo->local_depth < FileInfo->global_depth){
              
                // Store all records and re-Insert them
                bucket_records = pBInfo->num_records + 1;
                Record p_recs[pBInfo->num_records + 1];
                data = pBInfo + 1;
                // Copy records to p_recs
                memcpy(p_recs, data, (pBInfo->num_records) * sizeof(record));
                // Insert new record also
                p_recs[pBInfo->num_records] = record;
                // Create a new bucket
                if ((bf_code = BF_AllocateBlock(indexDesc, new_block)) != BF_OK)
                    return HT_ERROR;
                // Update num_blocks
                (FileInfo->num_blocks)++;
                data = BF_Block_GetData(new_block);
                // Update local depths
                pBInfo->local_depth++;
                new_pBinfo->local_depth = pBInfo->local_depth;
                new_pBinfo->num_records = 0;
                // Write new Bucket Info
                new_pBinfo = data;
                new_pBinfo[0] = BucketInfo;
                pBInfo->num_records = 0;
                // Set all dirty
                BF_Block_SetDirty(block);
                BF_Block_SetDirty(block_0);
                BF_Block_SetDirty(new_block);

                // Unpin block & new_block
                if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
                    return HT_ERROR;
                if ((bf_code = BF_UnpinBlock(new_block)) != BF_OK)
                    return HT_ERROR;

                // Update pointers to buckets
                HT_SplitHashTable(HashTable, s->block_num, FileInfo->num_blocks-1);
               
                // Re-insert all records iteratively
                Record rec;
                for (i = 0; i < bucket_records; i++){
                    rec = p_recs[i]; // Get a record

                    hash_index = HT_HashFunc(rec.id, FileInfo->global_depth); // Get bucket num
                    s = HT_GetHashTableBlockNum(HashTable, hash_index);
                    // Get Block num through hashtable
                    
                    if ((bf_code = BF_GetBlock(indexDesc, s->block_num, block)) != BF_OK)
                        return HT_ERROR;
                    data = BF_Block_GetData(block);
                    pBInfo = data;
                    // Insert record and update Bucket Info
                    data = pBInfo + 1;
                    prec = data;
                    prec[pBInfo->num_records] = rec;
                    pBInfo->num_records++;

                    // Set dirty and Unpin
                    BF_Block_SetDirty(block);
                    if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
                        return HT_ERROR;
                }
            }
            else { // Case 2: Local_depth = Global_depth - Need to expand
                // Store all records and re-Insert them

                bucket_records = pBInfo->num_records + 1;
                Record p_recs[pBInfo->num_records + 1];
                data = pBInfo + 1;
                // Copy records to p_recs
                memcpy(p_recs, data, (pBInfo->num_records) * sizeof(record));
                // Insert new record also
                p_recs[pBInfo->num_records] = record;
              
                // Create a new bucket
                if ((bf_code = BF_AllocateBlock(indexDesc, new_block)) != BF_OK)
                    return HT_ERROR;
                
                    
                // Update num_blocks
                (FileInfo->num_blocks)++;
                data = BF_Block_GetData(new_block);
                // Update local depths
                pBInfo->local_depth++;
                new_pBinfo->local_depth = pBInfo->local_depth;
                new_pBinfo->num_records = 0;
                // Write new Bucket Info
                new_pBinfo = data;
                new_pBinfo[0] = BucketInfo;
                pBInfo->num_records = 0;
                // Increment global depth for expansion
                FileInfo->global_depth++;

                // Set dirty's
                BF_Block_SetDirty(block);
                BF_Block_SetDirty(new_block);
                BF_Block_SetDirty(block_0);

                // Unpin block & new_block
                if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
                    return HT_ERROR;
                
                if ((bf_code = BF_UnpinBlock(new_block)) != BF_OK)
                    return HT_ERROR;
                
               
                // Expand & Update
                HT_ExpandHashTable(HashTable);
                HT_UpdateHashTable(HashTable, FileInfo->global_depth);
                HT_SplitHashTable(HashTable, s->block_num, FileInfo->num_blocks - 1);

                // Re-insert all records iteratively
                Record rec;
               
                for (i = 0; i < bucket_records; i++){
                    rec = p_recs[i]; // Get a record
                    hash_index = HT_HashFunc(rec.id, FileInfo->global_depth); // Hash Index
                    
                    // Get block num through hash index
                    s = HT_GetHashTableBlockNum(HashTable, hash_index);
                   
                    if ((bf_code = BF_GetBlock(indexDesc, s->block_num, block)) != BF_OK)
                        return HT_ERROR;
                    data = BF_Block_GetData(block);
                    pBInfo = data;
                    // Insert record and update Bucket Info
                    data = pBInfo + 1;
                    prec = data;
                    prec[pBInfo->num_records] = rec;
                    pBInfo->num_records++;

                    // Set dirty and Unpin
        
                    BF_Block_SetDirty(block);
                   
                    if ((bf_code = BF_UnpinBlock(block)) != BF_OK)
                        return HT_ERROR;
                    
                    
                }
            }
        }
    }
    // Destroy all blocks
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&block_0);
    BF_Block_Destroy(&new_block);

    return HT_OK;
}

HT_ErrorCode Print_HT_Bucket_Records(int indexDesc,LList *HashTable){
    LList* cursor = HashTable;
    void* data;
    HT_BucketInfo *BInfo;
    Record *prec;
    BF_Block *block;
    HT_ErrorCode code;
    BF_Block_Init(&block);

    do{
        if ((code = BF_GetBlock(indexDesc, cursor->block_num, block)) != BF_OK)
            return -1;
        data = BF_Block_GetData(block);
        BInfo = data;
        data = BInfo + 1;
        prec = data;
        // printf("Block %d: Records: %d Depth: %d\n",cursor->block_num, BInfo->num_records, BInfo->local_depth);
        for (int i = 0; i < BInfo->num_records; i++)
            printf("id: %d\n", prec[i].id);
        
        if ((code = BF_UnpinBlock(block)) != BF_OK)
            return code;
        cursor = cursor->next;
    }while(cursor != NULL);
    
}


HT_ErrorCode HT_PrintAllEntries(int indexDesc,LList* HashTable, int *id) {
    LList* cursor = HashTable;
    void* data;
    HT_BucketInfo *BInfo;
    Record *prec;
    BF_Block *block;
    HT_ErrorCode code;
    BF_Block_Init(&block);

    do{
        if ((code = BF_GetBlock(indexDesc, cursor->block_num, block)) != BF_OK)
            return -1;
        data = BF_Block_GetData(block);
        BInfo = data;
        data = BInfo + 1;
        prec = data;
        // printf("Block %d: Records: %d Depth: %d\n",cursor->block_num, BInfo->num_records, BInfo->local_depth);
        for (int i = 0; i < BInfo->num_records; i++){
            if(id == NULL){
                printf("id: %d, name: %s, surname: %s, city: %s\n", prec[i].id,prec[i].name,prec[i].surname,prec[i].city);
            }else if(prec[i].id == *id){
                printf("id: %d, name: %s, surname: %s, city: %s\n", prec[i].id,prec[i].name,prec[i].surname,prec[i].city);
            }
        }
        if ((code = BF_UnpinBlock(block)) != BF_OK)
            return code;
        cursor = cursor->next;
    }while(cursor != NULL);
    return HT_OK;
}

HT_ErrorCode HashStatistics(char* filename,LList *HashTable){
    int max = 0, min = 10, total = 0;
    float av = 0;
    LList* cursor = HashTable;
    void* data;
    HT_FileInfo* finfo;
    HT_BucketInfo *BInfo;
    Record *prec;
    BF_Block *block,*block_0;
    HT_ErrorCode code;
    int fd;
    BF_Block_Init(&block);
    BF_Block_Init(&block_0);
    if ((code = BF_OpenFile(filename,&fd)) != BF_OK)
            return -1;
    if ((code = BF_GetBlock(fd, 0, block_0)) != BF_OK)
            return -1;
    data = BF_Block_GetData(block_0);
    finfo = data;
    printf("Number of blocks of the file %d \n",finfo->num_blocks);

    do{
        if ((code = BF_GetBlock(fd, cursor->block_num, block)) != BF_OK)
            return -1;
        data = BF_Block_GetData(block);
        BInfo = data;
        data = BInfo + 1;
        prec = data;
        if(BInfo->num_records > max){
            max = BInfo->num_records;
        }
        if(BInfo->num_records < min){
            min = BInfo->num_records;
        }
        total += BInfo->num_records;
        if ((code = BF_UnpinBlock(block)) != BF_OK)
            return code;
        cursor = cursor->next;
    } while(cursor != NULL);

    av = (float) total / (finfo->num_blocks - 2);
    printf("Max number of records in a bucket %d\n", max);
    printf("Min number of records in a bucket %d\n", min);
    printf("Average number of records per bucket %f\n", av);

    if ((code = BF_UnpinBlock(block)) != BF_OK)
        return HT_ERROR;
    if ((code = BF_UnpinBlock(block_0)) != BF_OK)
            return HT_ERROR;
    BF_Block_Destroy(&block);
    BF_Block_Destroy(&block_0);                              
    return HT_OK;
}