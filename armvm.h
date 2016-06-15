/* Project "ARMed" Header File
 * This file contains the prototypes to all functions
 * in the ARMVM.
 * An ARM Virtual Machine, including assembler ("ARMler").
 * Emulates simple ARM commands using a GUI-based VM.
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/04/15
 * Date Modified:2011/06/17
 *                                                          */
#include <stdlib.h>
#include <stdio.h>
#include "armops.h"
#include "lnklists.h"

// Function Prototypes
bool ARMAssembleToFile(llist *lines, FILE *fpout);
								// Assemble n lines of ARM instructions
								// into 32-bit words and store in fpout
word ARMAssembleLine(char**);	// Convert tokenized ARM instruction to 
								// a 32-bit machine instruction
char* ARMDisassemble(dword);	// Covert 32-bit machine instruction to
								// a line of ARM code
bool ArmExecuteInstr(word);		// Execute a 32-bit machine instruction in ARM format
								// on the ARM Virtual Machine
bool ARMLoadFile(FILE *fpin);	// Loads a file; TODO: this must support "unlimited" Virtual Memory scheme
bool isCmpInstr(opcode);		// checks if operation is a cmp instruction
bool isMovInstr(opcode);		// checks if operation is a mov instruction
bool checkCondition(cond);		// Check if a condition is met (e.g. EQ has flag 'Z' set)
regn getReg(char*);				// Get the register number of a given register string
const char* getRegStr(regn); 	// Get the string representation of a given reg number
cond getCond(char*);			// Get the condition part of an ARM instruction
const char* getCondStr(cond); 	// Get the Condition's string based on the given number
opcode getOpCode(char*);		// Get the Opcode of an ARM instruction
const char* getOpCodeStr(opcode); // Get the Opcode's string based on the given number
itype getIType(char*);			// Get the instruction type of an ARM instruction
hword getConst(char*);			// Get a constant from a token (16-bit)
word getAddress(char*);		// Get a constant (address) from a token (24-bit); 
							// used for Branch instructions
char** tokenize(char*, char*, char*, char*, bool);	
								// Tokenize string into array of symbols
char** strnsplit(char**, char*, unsigned);
								// Split string into 2 parts at the point
								// at the specified index value
char* strcombine(char *dest, int size, char **tok, int n, 
				const char *sep, const char *delim);
								// Combine a series of tokens onto the end of a given
								// string, seperated by 'seperators' and ended with
								// 'delim' + '\0'
char* strupper(char*);		// Convert entire string to lower case
char* strrev(char*);		// Return the reverse of a string (without changing original)
void printw(word);			// Print a word as a binary string
