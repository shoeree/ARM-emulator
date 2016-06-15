/* Project "ARMed" Header File
 * Includes hash table functions and
 * data structure declaration, used for
 * storing labels.
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/04/18
 * Date Modified:2011/04/26
 *                                   */
#include "armdefs.def"
#include "lnklists.h"

typedef unsigned int uint;
// Hash Table Entry Structure
typedef struct {
	char label[LINE_MAX+1];
	word address;
	bool isResolved;
	llist *unresolvedInstr;
} HEntry;
 
// Hash Table Structure
typedef struct {
	uint size;
	uint capacity;
	HEntry **data;
} HTable;

// Function Prototypes
HTable* createHTable(uint size);
bool freeHTable(HTable *table);

HEntry* createHEntry(char *label, word address, bool isResolved);
bool freeHEntry(HEntry *entry);

bool hinsert(HTable *table, HEntry *entry, bool isAllowIncreaseCapacity); 
//bool hremove(HTable *table, char *label);
HEntry* hfind(HTable *table, char *label);
uint hash1(HTable *table, char *label);

// Linked List compare function
int llcmp_str(void *str1, void *str2);
