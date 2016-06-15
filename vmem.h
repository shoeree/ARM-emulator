/* Project "ARMed" Header File
 * Includes memory functions for the
 * virtual memory cache implementation.
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/06/15
 * Date Modified:2011/06/17
 *                                   */
#include "armdefs.def"

#ifndef VMEM_H
#define VMEM_H

// Definitions
// Page Table 
// |----[21]---|----[20]---|---[19]--|---[18-0]---|
// | Valid Bit | Dirty Bit | Ref Bit | Phys Addr  | Total Size = 22 bits
// |----------------------------------------------| 21-0
#define VALIDB	2097152	// Valid Bit 		2^21
#define DIRTYB	1048576	// Dirty Bit 		2^20
#define REFB	524288	// Reference Bit 	2^19
	// Note: REFB is used for if the page exists

#define PHYS_PAGE_NUMBER 0x0003FFFF // bits 18-0

// Function Prototypes
word getMemory(word);		// Get memory from (virtual) address specified
bool setMemory(word, word);	// Set memory of (virtual) address specified

// Copied itoa and reverse functions
void reverse(char*);
void itoa(int, char*);

#endif
