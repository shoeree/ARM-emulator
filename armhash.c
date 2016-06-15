/* Project "ARMed" Source File
 * Includes hash table functions and
 * data structure definitions, used for
 * storing labels.
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/04/18
 * Date Modified:2011/04/26
 *                                   */
#include <stdlib.h>
#include <string.h>
#include "armhash.h"

// Function Definitions

// Create a new hash table
HTable* createHTable(uint size) {
	HTable *table = malloc(sizeof(HTable));
	if (!table)
		return NULL; // failed to allocate memory
	table->capacity = size;
	table->size = 0;
	if (!(table->data = malloc(sizeof(HEntry)*size))) {
		free(table);
		return NULL; // failed to allocate memory for entries
	}
	return table;
}

// Free a hash table and its entries
bool freeHTable(HTable *table) {
	if (table == NULL)
		return false;
	for (int i=0; i<table->size; i++) {
		if (table->data[i]) free(table->data[i]);
	}
	return true;
}


// Create a new hash table entry
HEntry* createHEntry(char *label, word address, bool isResolved) {
	HEntry *entry = malloc(sizeof(HEntry));
	if (!entry)
		return NULL; // failed to allocate memory
	strncpy(entry->label, label, LINE_MAX);
	entry->address = address;
	entry->isResolved = isResolved;
	entry->unresolvedInstr = createLList(); // intialize empty list
	return entry;
}

// Free a hash table entry
bool freeHEntry(HEntry *entry) {
	if (entry == NULL)
		return false;
	lldelAll(entry->unresolvedInstr); // remove all list elements
	free(entry->unresolvedInstr); // delete list
	free(entry);
}

// Insert an entry into a hash table
bool hinsert(HTable *table, HEntry *entry, bool isAllowIncreaseCapacity) {
	if (entry == NULL || table == NULL)
		return false; // error: NULL arguments given
	// Check if table capacity must be increased (if allowed)
	if (isAllowIncreaseCapacity
		&& table->size + 1 > (int)((table->capacity/2)+0.5)) {
		// First double size of existing table
		HEntry **newEntries = malloc(sizeof(HEntry)*table->capacity*2);
		if (!newEntries) {
			return false; // failed to allocate memory for larger table
		}
		if (table->data) {
			// Copy entries to new data table
			for (int i=0; i<table->capacity; i++) {
				newEntries[i] = table->data[i];
			}
			// Free old data
			free(table->data);
			// Set table's data to point to new data
			table->data = newEntries;
			// Update capacity
			table->capacity = table->capacity * 2;
		}
	}
	// Attempt to insert the element using Hashing function
	bool isInserted = false;
	uint hres = hash1(table, entry->label);
	while (!isInserted) {
		int tmp = hres % table->capacity;
		if (table->data[tmp] == NULL) {
			table->data[tmp] = entry;
			isInserted = true;
		}
		else {
			hres *= 2;
		}
	}
	table->size++; // update current number of entries in table
	// Finished
	return true;
}

// Remove an entry from a hash table based on 'label'
/*bool hremove(HTable *table, char *label) {
	// not implemented
	return false;
}*/

// Find an entry in a hash table based on 'label'
HEntry* hfind(HTable *table, char *label) {
	HEntry *foundEntry = NULL;
	if (table == NULL || label == NULL) {
		return foundEntry; // error; NULL parameter(s) given
	}
	bool isFound = false;
	uint hres1 = hash1(table, label);
	uint hres = hres1;
	while (!isFound) {
		int tmp = hres % table->capacity;
		if (table->data[tmp] && 
			strncmp(table->data[tmp]->label, label, LINE_MAX) == 0) {
			foundEntry = table->data[tmp];
			isFound = true;
		}
		else {
			hres *= 2;
		}
		// If we return to the original value again,
		// then the entry was not found
		if (hres1 == (hres % table->capacity))
			break;
	}
	return foundEntry;
}

// Hashing function
uint hash1(HTable *table, char *label) {
	return (uint)(label[0] % table->capacity);
}

// Linked List compare function
int llcmp_str(void *str1, void *str2) {
	return (strcmp((const char*)str1, (const char*)str2) == 0);
}
