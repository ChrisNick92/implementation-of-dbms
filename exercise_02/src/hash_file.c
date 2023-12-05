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
  //insert code here
  return HT_OK;
}

/* HASH TABLE UTILS */

int HT_HashFunc(int k, int n_total, int n_bits){
    int mask, mask_2;
    mask = (~((~0) << n_total)) & k;
    mask_2 = (~((~0) << n_total)) & (~0);

    return (mask & (~(mask_2 >> n_bits))) >> (n_total - n_bits);
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
    int index, list_length;

    index = 1;
    list_length = HT_HashTableLen(root);

    while (q->next != NULL){
        if (HT_HashFunc(index, list_length, depth - 1) == HT_HashFunc(index - 1, list_length, depth - 1 )) {
            q->block_num = (q->prev)->block_num;
        }
        else
            q->block_num = (q->next)->block_num;
        
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
    if ((code = BF_CreateFile(filename)) != BF_OK)
        return HT_ERROR;

    // Open file
    if ((code = BF_OpenFile(filename, &fd)) != BF_OK)
        return HT_ERROR;
    

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

HT_ErrorCode HT_CloseFile(int indexDesc) {
    BF_Block *block;
    BF_Block_Init(&block);
    BF_ErrorCode code;

    if ((code = BF_GetBlock(indexDesc, 0, block)) != BF_OK)
        return HT_ERROR;
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
    int bucket_num;
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
    bucket_num = HT_HashFunc(record.id, FileInfo->global_depth, FileInfo->global_depth);
    // Find block num and read
    s = HT_GetHashTableBlockNum(HashTable, bucket_num);

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
                if ((bf_code == BF_UnpinBlock(new_block)) != BF_OK)
                    return HT_ERROR;

                // Update pointers to buckets
                HT_SplitHashTable(HashTable, s->block_num, FileInfo->num_blocks-1);

                // Re-insert all records recursively
                for (i = 0; i < bucket_records; i++){
                    if ((ht_code = HT_InsertEntry(indexDesc, HashTable, p_recs[i])) != HT_OK)
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
                if ((bf_code == BF_UnpinBlock(new_block)) != BF_OK)
                    return HT_ERROR;

                // Expand & Update
                HT_ExpandHashTable(HashTable);
                HT_UpdateHashTable(HashTable, FileInfo->global_depth);

                // Re-insert all records recursively
                for (i = 0; i < bucket_records; i++){
                    if ((ht_code = HT_InsertEntry(indexDesc, HashTable, p_recs[i])) != HT_OK)
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

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

