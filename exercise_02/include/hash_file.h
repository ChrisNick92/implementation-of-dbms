#ifndef HASH_FILE_H
#define HASH_FILE_H

typedef enum HT_ErrorCode {
  HT_OK,
  HT_ERROR
} HT_ErrorCode;

typedef struct Record {
	int id;
	char name[15];
	char surname[20];
	char city[20];
} Record;

/* Η δομή HT_FileInfo κρατάει πληροφορία για το αρχείο κατακερματισμού.
 * Η μεταβλητή hash_table_block_num δείχνει στο πρώτο block του ερευτηρίου.
 * Η μεταβλητή max_nodes_per_block αντιστοιχεί στο πόσα nodes του ερευτηρίου χωράει το ένα block
 * Η μεταβλητή num_node_blocks αντιστοιχεί στο πλήθος των block που αντιστοιχούν σε Nodes,
 * ενώ η μεταβλητή max_records_per_bucket πόσα records το κάθε bucket.
 */
typedef struct HT_FileInfo
{
    int global_depth;
    int hash_table_block_num;
    int num_node_blocks;
    int max_nodes_per_block;
    int max_records_per_bucket;

} HT_FileInfo;

/* Η δομή HT_HashNodeInfo κρατάει πληροφορία για έναν μπλοκ του ερευτηρίο.
 * Η μεταβλητή next_block_num δείχνει στο επόμενο block του ερευτηρίου, ενώ
 * η μεταβλητή num_block_nodes αντιστοιχεί στο πλήθος των εγγραφών που έχει
 * το block.
 */
typedef struct HT_HashNodeInfo
{
    int next_block_num;
    int num_block_nodes;

} HT_HashNodeInfo;


HT_FileInfo * HT_GetFileInfo(int indexDesc);

/* Δομή που κρατάει πληροφορία για τα buckets */
typedef struct HT_BucketInfo
{
    int local_depth;
    int num_records;
} HT_BucketInfo;


/*
 * Η συνάρτηση HT_Init χρησιμοποιείται για την αρχικοποίηση κάποιον δομών που μπορεί να χρειαστείτε. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται HT_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_Init();

/*
 * Η συνάρτηση HT_CreateIndex χρησιμοποιείται για τη δημιουργία και κατάλληλη αρχικοποίηση ενός άδειου αρχείου κατακερματισμού με όνομα fileName. 
 * Στην περίπτωση που το αρχείο υπάρχει ήδη, τότε επιστρέφεται ένας κωδικός λάθους. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HΤ_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_CreateIndex(
	const char *fileName,		/* όνομααρχείου */
	int depth
	);


/*
 * Η ρουτίνα αυτή ανοίγει το αρχείο με όνομα fileName. 
 * Εάν το αρχείο ανοιχτεί κανονικά, η ρουτίνα επιστρέφει HT_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_FileInfo* HT_OpenIndex(
	const char *fileName, 		/* όνομα αρχείου */
  int *indexDesc            /* θέση στον πίνακα με τα ανοιχτά αρχεία  που επιστρέφεται */
	);

/*
 * Η ρουτίνα αυτή κλείνει το αρχείο του οποίου οι πληροφορίες βρίσκονται στην θέση indexDesc του πίνακα ανοιχτών αρχείων.
 * Επίσης σβήνει την καταχώρηση που αντιστοιχεί στο αρχείο αυτό στον πίνακα ανοιχτών αρχείων. 
 * Η συνάρτηση επιστρέφει ΗΤ_OK εάν το αρχείο κλείσει επιτυχώς, ενώ σε διαφορετική σε περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_CloseFile(
	int indexDesc 		/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	);

/*
 * Η συνάρτηση HT_InsertEntry χρησιμοποιείται για την εισαγωγή μίας εγγραφής στο αρχείο κατακερματισμού. 
 * Οι πληροφορίες που αφορούν το αρχείο βρίσκονται στον πίνακα ανοιχτών αρχείων, ενώ η εγγραφή προς εισαγωγή προσδιορίζεται από τη δομή record. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HT_OK, ενώ σε διαφορετική περίπτωση κάποιος κωδικός λάθους.
 */
HT_ErrorCode HT_InsertEntry(
	int indexDesc,	/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	Record record		/* δομή που προσδιορίζει την εγγραφή */
	);

/*
 * Η συνάρτηση HΤ_PrintAllEntries χρησιμοποιείται για την εκτύπωση όλων των εγγραφών που το record.id έχει τιμή id. 
 * Αν το id είναι NULL τότε θα εκτυπώνει όλες τις εγγραφές του αρχείου κατακερματισμού. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HP_OK, ενώ σε διαφορετική περίπτωση κάποιος κωδικός λάθους.
 */
HT_ErrorCode HT_PrintAllEntries(
	int indexDesc,	/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	int *id 				/* τιμή του πεδίου κλειδιού προς αναζήτηση */
	);


/* Δομές που μοντελοποιούν το ευρετήριο όταν τρέχει στη μνήμη */


/*Η Δομή που μοντελοποιεί το ευρετήριο (διπλά συνδεδμένη λίστα)*/
typedef struct LList
{
    /* data */
    struct LList *prev, *next;
    int block_num;

} LList;

/* Η συνάρτηση hash_func δέχεται έναν ακέραιο
 * και επιστρέφει έναν ακέραιο που αντιστοιχεί
 * στα πρώτα m bits (least significant) του k.
 */

int HT_HashFunc(int k, int n_total, int n_bits);

/*Η συνάρτηση LList_Init αρχικοποιεί τη διπλά συνδεδμένη λίστα.*/
LList * HT_LList_Init(void);

/* Η συνάρτηση expand_hash_table διπλασιάζει
 * το μέγεθος του table προσθέτοντας ενδιάμεσους
 * κόμβους.
 */
void HT_ExpandHashTable(LList *root);

/* Η συνάρτηση update_hash_table ανανεώνει
 * το hash table αφού γίνει expand. Κάθε
 * νέος κόμβος δείχνει εκεί που δείχνουν
 * και οι κόμβοι με τους οποίους ταιριάζει
 * στα πρώτα global depth bits -
 * use always (and only) after expand.
 */

void HT_UpdateHashTable(LList *root, int depth);

/* Μέγεθος του hash table */
int HT_HashTableLen(LList *root);

/* For debug */
void HT_PrintHashTable(LList *root);

/* Η συνάρτηση split_hash_table χωρίζει στη μέση
 * τους δείκτες που δείχνουν στο target_block_num
 * και τους μισούς τους αντιστοιχεί στο new_block_num.
 * Η split χρησιμοποιείται όταν μια νέα εγγραφή δεν 
 * χωράει στον κάδο που να μπει.
 */

void HT_SplitHashTable(LList *root, int target_block_num, int new_block_num);

/* Η συνάρτηση HT_HashTable_toList δέχεται ως όρισμα το file descriptor
 * ενός αρχείου κατακερματισμού και επιστρέφει το hash table σε μια
 * διπλά συνδεδεμένη λίστα.
 */
LList *HT_HashTable_toList(int indexDesc);

#endif // HASH_FILE_H
