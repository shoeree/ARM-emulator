/* Project "ARMed" Source File - Main
 * Main source file, which assembles and executes
 * ARM source code on the ARMVM.
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/04/17
 * Date Modified:2011/04/26
 *          
                                                */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "armvm.h"
#include "lnklists.h"
#include "vmem.h"

// Function Prototypes
bool ARMDebug(FILE *fpin);
bool outputToFile(FILE *fpin, FILE *fpout);
void showRegs();

// Define Modes
#define ME 0	// error
#define MO 1	// assemble & output
#define MR 2	// run non-assembled file
#define MX 4	// run assembled file
#define MD 8	// show disassembly of file
byte exmode = 0;

// External variables
extern word regs[];
extern word RegStr[];
extern word flags[];
extern word memory[];
extern word segs[];
extern bool isJump;
extern dword nInstructions;

// Seperators, Delimiters, Brackets
extern char sep[];
extern char delim[];
extern char grp[];

#define DEFAULT_TEMP_FNAME "TMP_ARMVM_FILE_0X19S05A30RM"

// Main
int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("usage: ([input file]) [ARGS]\nWhere [ARGS] =\n-o [binary-Output-file]\
\n-r [file-to-Run]\n-x [binary-file-to-eXecute]\n-d [file-to-Disassemble]\
\n *** Only use 1 option at a time ***\n");
		return 0; // Exit
	}
	
	FILE *fpin  = NULL;
	FILE *fpout = NULL;
	bool isFlag = false; // was a '-' flag in the last argument?
	for (int i=1; i<argc; i++) {
		if (argv[i][0] == '-') {
			argv[i][1] = toupper(argv[i][1]);
			if (argv[i][1] == 'O') {
				exmode = MO;
			} else if (argv[i][1] == 'R') {
				exmode = MR;
			} else if (argv[i][1] == 'X') {
				exmode = MX;
			} else if (argv[i][1] == 'D') {
				exmode = MD;
			}
			isFlag = true;
		}
		else if (argv[i][0] != '-') {
			if (isFlag) {
				if (exmode & MO) {
					fpout = fopen(argv[i], "wb");
				} else {
					if (!fpin) fpin = fopen(argv[i], "r");
				}
				isFlag = false;
			} else {
				if (!fpin) fpin = fopen(argv[i], "r");
			}
		}
	}
	
	// Check for file read error
	if (!fpin) {
		printf("Error: Failed to open file.\n");
		if (fpout) fclose(fpout);
		return 0; // Exit
	}
	
	// Assemble & Output Mode
	if (exmode & MO) {
		outputToFile(fpin, fpout);
	}
	// Run-Directly Mode
	else if (exmode & MR) {
		// Create temporary object file
		FILE *tmpf = fopen(DEFAULT_TEMP_FNAME, "w");
		if (!outputToFile(fpin, tmpf)) {
			printf("Error opening file.\n");
		}
		fclose(fpin);
		fclose(tmpf);
		// Read from temporary file
		fpin = fopen(DEFAULT_TEMP_FNAME, "r");
		if (!ARMDebug(fpin)) {
			printf("Error executing file.\n");
		}
		// Close temporary file and remove it
		fclose(fpin);
		remove(DEFAULT_TEMP_FNAME);
	}
	// Execute File Mode
	else if (exmode & MX) {
		ARMDebug(fpin);
	}
	// Show Disassembly
	else if (exmode & MD) {
		word buf = 0;
		printf("Disassembly of file...\n\n");
		while (fread((void*)&buf, sizeof(buf), 1, fpin)) {
			char *line = ARMDisassemble(buf);
			if (line != NULL) {
				printf("%s\n", line);
			}
			free(line);
		}
	}
	
	
	if (fpin)  fclose(fpin);
	if (fpout) fclose(fpout);
	
	// Exit
	return 0;
}

//
// Run a Debug-style emulation of an ARM processor
// Show Regiters, flags and current instruction at each step
//
// Commands are s:step, c:continue, g:go, q:quit
//
// Return false when file has finished executing
// or if there is an error
//
bool ARMDebug(FILE *fpin) {
	// Load code into memory
	ARMLoadFile(fpin);
	memset(regs, 0, sizeof(word)*ARMVM_NREGS); // clear regs
	regs[SP] = DEFAULT_STACK_MEMORY-1;	// Set Stack Pointer to top of stack
	// Loop until program finished or user quits
	#define QUIT		0x00
	#define STEP 		0x01
	#define GO			0x03
	#define CONTINUE 	0x04
	byte runMode = STEP;
	while (runMode) {
		isJump = false; // reset jump flag
		word currInstr = getMemory(regs[PC]); // get instruction from memory according to PC
		if (runMode & STEP) {
			showRegs();
			printf("%08X:%08X\t", regs[PC], currInstr);
			if (regs[PC] < nInstructions)
				// Note: CSEG stores the TOTAL number of instructions in the program.
				// The "text segment" is going to always be <= 4KB (max # of pages in memory = 4)
				printf("%s\n", ARMDisassemble(currInstr));
			else {
				printf("END\n-----------\nEnd of file reached.\n");
				runMode = QUIT;
			}
			// Check current mode and get user input if necessary
			if (runMode == STEP) {
				// prompt for user input
				char userInput[LINE_MAX] = {0};
				printf(" >> ");
				fgets(userInput, LINE_MAX-1, stdin);
				if (strncasecmp(userInput, "step", 4) == 0 ||
					strncasecmp(userInput, "s", 1) == 0) {
					runMode = STEP;
				}
				else if (strncasecmp(userInput, "continue", 8) == 0 ||
					strncasecmp(userInput, "c", 1) == 0) {
					runMode = CONTINUE;
				}
				else if (strncasecmp(userInput, "go", 2) == 0 ||
					strncasecmp(userInput, "g", 1) == 0) {
					runMode = GO;
				}
				else if (strncasecmp(userInput, "quit", 4) == 0 ||
					strncasecmp(userInput, "q", 1) == 0) {
					runMode = QUIT;
				}
				else {
					printf("Commands are:\
\n'step', 's'     : Step through program line-by-line, viewing registers and flags.\
\n'continue', 'c' : Skip through program until the end.\
\n'go', 'g'       : Skip through program until the end, viewing registers and flags.\
\n'quit', 'q'     : Quit debugging the program and exit.\n");
				}
			}
		}
		ARMExecuteInstr(currInstr);
		if (!isJump) regs[PC]+=1; // PC + 1 word (= 4bytes = 32bits)
	}
	// Done - success
	return true;
}

// Output result of assembly to file
bool outputToFile(FILE *fpin, FILE *fpout) {
	if (fpin == NULL || fpout == NULL)
		return false; // error
	llist *lines = createLList();
	char buf[LINE_MAX] = {0};
	while (fgets(buf, sizeof(buf)/sizeof(buf[0]), fpin)) {
		char *str = malloc(sizeof(char)*LINE_MAX);
		memset(str, 0, sizeof(char)*(strlen(buf)+1));
		strcpy(str, buf);
		lladdTail(lines, createLLNode((void*)str));
	}
	if (!(ARMAssembleToFile(lines, fpout))) {
		printf("Error: Failed to assemble file.\n");
		lldelAll(lines);
		free(lines);
		return false; // error
	}
	lldelAll(lines);
	free(lines);
	// Done
	return true;
}

// show registers and flags
void showRegs() {
	printf("REGISTERS:\t");
	for (int i=0; i<ARMVM_NREGS; i++) {
		printf("%s:0x%08X ", RegStr[i], regs[i]);
		if ((i+1) % 4 == 0) printf("\n\t\t");
	}
	printf("FLAGS:\t {ZCNV} = {%1d%1d%1d%1d}\n",
		flags[ZF], flags[CF], flags[NF], flags[VF]);
}
