/* Project "ARMed" Source File
 * Includes memory functions for the
 * virtual memory cache implementation.
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/06/15
 * Date Modified:2011/06/28
 *                                   */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "vmem.h"

// Memory
extern word memory[];

// Page Table; 4GB/1KB in size = 4194304 pages available
word page_table[M4GB/M1KB];

// Retrieve memory from main/secondary memory
// and update the page table
//
// Returns a word if != 0 if the word was found in main memory,
// or a 0 if the memory was not found (but the table was updated).
//
// Note: If this function returns 0, then the last command which called
// this function will have to be executed AGAIN to get the memory needed.
// 
word getMemory(word addr) {
	word page 		= addr / M1KB; // get VIRTUAL page #
	word offset 	= addr % M1KB; // specific word in page
	word phys_page 	= page_table[page] & PHYS_PAGE_NUMBER; // get physical page number from entry
	
	// Check if page is valid and thus is in main memory
	if (page_table[page] & VALIDB) {
		return memory[(phys_page*M1KB)+offset];
			// Get word from one of the memory's stored pages
	}
	// Otherwise, page is not in main memory, and we must load it
	else {
		word pindex;
		bool isFound = false;
		srand(time(NULL));
		int randpage = rand() % 4; // TODO: figure out how to do a LRU operation!!!!
		// Find matching page in page table
		for (pindex=0; pindex < M4GB/M1KB; pindex++) { // This is a large for loop, and may cost a lot of time
			if (((page_table[pindex] & PHYS_PAGE_NUMBER) == randpage)
				&& (page_table[pindex] & VALIDB)) {
				isFound = true;
				break;
			}
		}
		// Check if page to be replaced was modified (but only do something if it exists)
		if (isFound && (page_table[pindex] & DIRTYB)) {
			// Generate page file name
			char fname[27] = {0}, pnum[20] = {0};
			itoa(pindex, pnum);
			strncat(fname, "PG/Mem", 7);
			strncat(fname, pnum, 20);
			// Write data to page file
			FILE *opage = fopen(fname, "wb");
			fwrite(&memory[randpage*M1KB], M1KB, 1, opage);
			// Close page file
			fclose(opage);
			// Clear valid bit
			page_table[pindex] &= ~VALIDB;
			// Set new physical address of page to be outside of memory
			page_table[pindex] &= PHYS_PAGE_NUMBER & pindex; 
				// Observation: It is possible that the address on disk equals the physical
				// address in main memory, e.g. if the file's name is "Mem0" then the physical
				// address is = 0, which is the same as if the page existed in memory[0*M1KB].
				// TODO: does this do anything weird with the memory "swapping" scheme?
		}
		// Replace page
		// Get page file name
		char fname[27] = {0}, pnum[20] = {0};
		itoa(page, pnum);
		strncat(fname, "PG/Mem", 7);
		strncat(fname, pnum, 20);
		FILE *ipage = fopen(fname, "rb");
		// Read from page file
		fread(&memory[randpage*M1KB], M1KB, 1, ipage);
		// Close page file
		fclose(ipage);
		// Set Valid bit
		page_table[page] |= VALIDB;
		// Set new physical address of page to be in memory
		page_table[page] |= PHYS_PAGE_NUMBER & randpage;
		
		// return 0 - meaning there was a "miss"
		return 0;
	}
}

// Set data found in memory at given location to the supplied value
//
// Returns 1 if the data was successfully set,
// and 0 if the data was not present in main memory and had to be swapped
// first.
//
// Note: if this function returns 0, then it means that the last operation
// which called this function will (probably) have to be executed AGAIN.
//
bool setMemory(word addr, word val) {
	word page 		= addr / M1KB; // get VIRTUAL page #
	word offset 	= addr % M1KB; // specific word in page
	word phys_page 	= page_table[page] & PHYS_PAGE_NUMBER; // get physical page number from entry
	
	// Check if page is valid and thus is in main memory
	if (page_table[page] & VALIDB) {
		memory[(phys_page*M1KB) + offset] = val;
			// Set a word in memory to value given
		page_table[page] |= DIRTYB; // Set dirty ('modified') bit
		return true; // "hit"
	}
	// Otherwise, page is not in main memory, and we must load (or create) it
	else {
		word pindex = 0;
		bool isFound = false;
		srand(time(NULL));
		int randpage = rand() % 4; // TODO: figure out how to do a LRU operation!!!!
		// Find matching page in page table
		for (; pindex<M4GB/M1KB; pindex++) { // This is a large for loop, and may cost a lot of time
			if (((page_table[pindex]&PHYS_PAGE_NUMBER) == randpage)
				&& (page_table[pindex]&VALIDB)) {
				isFound = true;
				break;
			}
		}
		// Check if page to be replaced was modified
		if (isFound && (page_table[pindex] & DIRTYB)) {
			// Generate page file name
			char fname[27] = {0}, pnum[20] = {0};
			itoa(pindex, pnum);
			strncat(fname, "PG/Mem", 7);
			strncat(fname, pnum, 20);
			// Write data to page file
			FILE *opage = fopen(fname, "wb");
			fwrite(&memory[randpage*M1KB], M1KB, 1, opage);
			// Close page file
			fclose(opage);
			// Clear valid bit
			page_table[pindex] &= ~VALIDB;
			// Set new physical address of page to be outside of memory
			page_table[pindex] &= PHYS_PAGE_NUMBER & pindex; 
				// Observation: It is possible that the address on disk equals the physical
				// address in main memory, e.g. if the file's name is "Mem0" then the physical
				// address is = 0, which is the same as if the page existed in memory[0*M1KB].
				// TODO: does this do anything weird with the memory "swapping" scheme?
		}
		// Replace/Create page
		// Get page file name
		char fname[27] = {0}, pnum[20] = {0};
		itoa(page, pnum);
		strncat(fname, "PG/Mem", 7);
		strncat(fname, pnum, 20);
		// Create page if REFB (reference bit) is not set
		if ((page_table[page] & REFB) == 0) {
			page_table[page] |= PHYS_PAGE_NUMBER & randpage;
			page_table[page] |= REFB;
			page_table[page] |= VALIDB;
		}
		// Swap in new page from secondary memory (disk)
		else {
			FILE *ipage = fopen(fname, "rb");
			// Read from page file
			fread(&memory[randpage*M1KB], M1KB, 1, ipage);
			// Close page file
			fclose(ipage);
			// Set Valid bit
			page_table[page] |= VALIDB;
			// Set new physical address of page to be in memory
			page_table[page] |= PHYS_PAGE_NUMBER & randpage;
		}
		return false; // "miss"
	}
}

/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;
 
    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
