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

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

