/* Project "ARMed" Source File
 * This file contains the DEFINITIONS of the PROTOTYPES
 * found in armvm.h.
 * An ARM Virtual Machine, including assembler ("ARMler").
 * Emulates simple ARM commands using a console-based VM.
 *
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 *
 * Date Created: 2011/04/15
 * Date Modified:2011/06/28
 *                                                          */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "armvm.h"
#include "armhash.h"
#include "vmem.h"

// Condition Strings
#define X(a) #a,
const char *CondStr[] = {
	ARMVM_CONDITIONS
};
#undef X

// Reg Strings
#define X(a) #a,
const char *RegStr[] = {
	ARMVM_REGS
};
#undef X

// Opcodes
const char *OpCodeStr[] = {
	// DP
	"AND", "EOR", "SUB", "RSB",	//   0  1  2  3
	"ADD", "ADC", "SBC", "RSC", //   4  5  6  7
	"TST", "TEQ", "CMP", "CMN", //   8  9 10 11
	"ORR", "MOV", "BIC", "MVN", //  12 13 14 15
	// DT
	"NOP", "NOP", "NOP", "NOP", //  16 17 18 19
	"NOP", "NOP", "NOP", "NOP", //  20 21 22 23
	"LDR", "STR", "NOP", "NOP", //  24 25 26 27
	// B
	"B", "BL"					//  28 29
};

// Function Pointer Arrays
// Data Processing
typedef void (*ARM_DPFUNC)(regn, regn, word, bool);
ARM_DPFUNC DPFunctions[] = {
	ARM_AND, ARM_EOR, ARM_SUB, ARM_RSB,
	ARM_ADD, ARM_ADC, ARM_SBC, ARM_RSC,
	ARM_TST, ARM_TEQ, ARM_CMP, ARM_CMN,
	ARM_ORR, ARM_MOV, ARM_BIC, ARM_MVN
};
// Data Transfer
typedef void (*ARM_DTFUNC)(regn, regn, word);
ARM_DTFUNC DTFunctions[] = {
	ARM_LDR, ARM_STR
};
// Branch
typedef void (*ARM_BRFUNC)(word);
ARM_BRFUNC BRFunctions[] = {
	ARM_B, ARM_BL
};

// Seperators, Delimiters, Brackets
char sep[] = {' ', ',', '\t', '\0'};
char delim[] = {';', '\r', '\n', '\0'};
char grp[] = {'[', ']', '\0'};

// Symbol Table for Labels
// Every label that is used for branching goes in this
// table; if a label is placed in 2 locations, then there is
// an error
HTable *symbolTable = NULL; // initially null

// Virtual Condition Flags (4 flags = VCNF)
bool flags[4] = {0};

// Virtual Registers (16 total)
word regs[16] = {0};

// Virtual Memory (in words)
word memory[DEFAULT_MAIN_MEMORY];

// Number of instructions in program
dword nInstructions;

// Stack Memory (in words)
word stackMem[DEFAULT_STACK_MEMORY];

// Global 'jump' flag
// if it's set, then a branch operation is being executed
bool isJump = false;

// Assemble lines into a series of 32-bit words,
// which are written to file pointed to by fpout
//
// NOTE: Increments the PC register for each assembled line,
// and stores the instruction in the 'text segment'
// at the index given by the value in PC
//
// Returns true upon success and false otherwise
//
#define DBG_ARMASSEMBLETOFILE 0
bool ARMAssembleToFile(llist *lines, FILE *fpout) {
	if (lines == NULL || lines->size == 0 || fpout == NULL)
		return false; // error; null or invalid parameters
	// tokenize and assemble each line
	int nLines = lines->size;
	int i = 0;
	for (regs[PC] = 0;i < nLines; i++) {
		isJump = false; // reset jump flag
		if (DBG_ARMASSEMBLETOFILE) {
			printf("ARMAssembleToFile: Assembling code:\n\t%s",
				(char*)lines->head->data);
		}
		char **tokens = tokenize((char*)lines->head->data, sep, delim, grp, true);
		word instr = ARMAssembleLine(tokens);
		if (instr != UNDEFINED_INSTRUCTION) {
			memory[regs[PC]] = instr;
			if (DBG_ARMASSEMBLETOFILE) {
				printf("ARMAssembleToFile: memory[%6lu]\t", regs[PC]);
				printw(memory[regs[PC]]);
			}
			if (!isJump) regs[PC]++; // increment program counter
		}
		if (tokens) free(tokens);
		lldelHead(lines);
	}
	fwrite((void*)memory, sizeof(word), regs[PC], fpout);
		// Source, size of element, number of elements, destination
	return true;
}

// Convert tokenized ARM instruction into
// a 32-bit machine instruction
// Returns 0 if there is an error, and the 32-bit machine
// instruction corresponding to the tokens upon success.
#define DBG_ARMASSEMBLE 0
word ARMAssembleLine(char **tok) {
	if (tok == NULL)
		return UNDEFINED_INSTRUCTION; // Failure
	word instr = 0;
	int reg = 0;				// register
	itype type = INVALID_ITYPE;	// instruction type
	cond cnd = INVALID_COND;	// condition
	opcode op = NOP;			// opcode
	hword C = 0; 				// 12-bit constant
	word A = 0;					// 32-bit address (constant)
	bool hasLabel = false;
	char *opcond[2] = {0}; 		// Operation+Condition (e.g. "ADDEQ")

	int tok0len = strlen(tok[0]);
	// A LABEL can either be BEFORE an operation
	// 	e.g.	loop:	ADD r0, r1, r2 ; ...
	// or on its own line
	// 	e.g. 	loop:
	//			ADD r0, r1, r2 ; ...
	// The ARMVM must check if the first token has a trailing ':'.
	// If it does, then we must add the label to the symbol
	// table or update it,
	// and then we increment tok (to ignore the first token)
	if (tok[0][tok0len-1] == ':') {
		hasLabel = true;
		tok[0][tok0len-1] = '\0'; // remove ':'
		// Create table if it doesn't exist
		if (symbolTable == NULL) {
			symbolTable = createHTable(DEFAULT_TABLE_SIZE);
		}
		// Try to find the label
		HEntry *entry = hfind(symbolTable, tok[0]);
		if (entry) {
			if (entry->isResolved) {
				printf("Error: Label '%s' appears twice.\n", tok[0]);
				return UNDEFINED_INSTRUCTION;
			}
			else {
				entry->address = regs[PC];
				entry->isResolved = true;
				// Go through every unresolved instruction, and update
				// their address fields
				llnode *np = entry->unresolvedInstr->head;
				while (np != NULL) {
					memory[*(uint*)np->data] |= entry->address - *(uint*)np->data;
					free(entry->unresolvedInstr->head->data); // free the copied data
						// TODO: make SURE that this is safe (doesn't cause seg. fault)
					lldelHead(entry->unresolvedInstr);
					np = entry->unresolvedInstr->head;
				}
			}
		}
		// Insert a new label into the hash table
		else {
			hinsert(symbolTable, createHEntry(tok[0], regs[PC]+1, true), true);
		}
		free(tok[0]);
		tok++; /* Might cause seg. fault when we free tokens later... */
	}
	// Stop if the first token is empty; it shouldn't
	// ever be empty for valid instructions
	if (strcmp(tok[0], "") == 0) {
		if (hasLabel) isJump = true;
		return UNDEFINED_INSTRUCTION;
	}
	// For now, we assume that there are instructions with length = 3 max

	if (tok[0] && tok0len-2 > 0 &&
		(getCond(strnsplit(opcond, tok[0], tok0len-2)[1]) != INVALID_COND)) {
			if (DBG_ARMASSEMBLE) printf("ARMAssemble: Splitting first token;\n\t\
opcond = {%s, %s}\n", opcond[0], opcond[1]);
	}
	else {
		if (opcond[0]) free(opcond[0]);
		if (opcond[1]) free(opcond[1]);
		opcond[0] = malloc(sizeof(char)*tok0len);
		memset(opcond[0], 0, sizeof(char)*tok0len);
		strncpy(opcond[0], tok[0], tok0len+1);
		opcond[1] = NULL;
		if (DBG_ARMASSEMBLE) printf("ARMAssemble: NOT splitting first token;\n\t\
opcond = {%s, %s}\n", opcond[0], opcond[1]);
	}
	// Get instruction type
	type = getIType(opcond[0]);
	// Data Processing type
	if (type == DP) {
		// Get condition code (e.g. EQ = 0)
		if (opcond[1] && (cnd = getCond(opcond[1])) != INVALID_COND)
			instr |= cnd << 28;
		else
			instr |= 0xE0000000; // implicit "ALWAYS" condition
		// Get instruction Opcode (e.g. ADD = 4)
		if ((op = getOpCode(opcond[0])) != NOP)
			instr |= op << 21;
		else {
			// Undefined Instruction!
			// Free opcond array
			if (opcond) {
				if (opcond[0]) free(opcond[0]);
				if (opcond[1]) free(opcond[1]);
			}
			if (hasLabel) isJump = true; // do not increment PC!
			return UNDEFINED_INSTRUCTION;
		}

		bool cestCmp = isCmpInstr(op);
		bool cestMov = isMovInstr(op);
		// Parameter #1 - destination (Rd)
			// not used in cmp
		if (!cestCmp && (reg = getReg(tok[1])) != -1)
			instr |= reg << 12;

		// Parameter #2 - source/base (Rn)
			// not used in mov
		if (!cestMov && (reg = getReg(
					tok[2-(cestCmp ? 1:0)])) != -1)
			instr |= reg << 16;

		// Parameter #3 - offset/constant (Operand2)
		if ((reg = getReg(
				tok[3-(cestCmp||cestMov ? 1:0)])) != -1)
			instr |= reg;
		else if (((C = getConst(
					tok[3-(cestCmp||cestMov ? 1:0)])) & CONST_12_ERR) == 0) {
				instr |= C | 0x02000000; // Set "I" (Immediate) bit
				if (DBG_ARMASSEMBLE) printf("ARMassemble; Operand2 is a constant.\n");
		}
	}

	// Data Transfer type
	else if (type == DT) {
		instr |= 0x04000000; // Set instruction format to 1 (DT type)
		// Get condition code (e.g. EQ = 0)
		if (opcond[1] && (cnd = getCond(opcond[1])) != INVALID_COND)
			instr |= cnd << 28;
		else
			instr |= 0xE0000000; // implicit "ALWAYS" condition
		// Get instruction Opcode (e.g. LDR = 24)
		if ((op = getOpCode(opcond[0])) != NOP)
			instr |= op << 20;
		else {
			// Undefined Instruction!
			// Free opcond array
			if (opcond) {
				if (opcond[0]) free(opcond[0]);
				if (opcond[1]) free(opcond[1]);
			}
			if (hasLabel) isJump = true; // do not increment PC!
			return UNDEFINED_INSTRUCTION;
		}

		char **addrParam = NULL;
		// Get Base Address + Offset from third token (parameter #2)
		addrParam = tokenize(tok[2], sep, delim, grp, true);

		// Parameter #1 - destination (Rd)
		if ((reg = getReg(tok[1])) != -1)
			instr |= (reg << 12);

		// Parameter #2 - source/base (Rn)
		if (addrParam[1] && (reg = getReg(addrParam[0])) != -1)
			instr |= (reg << 16);
		else if ((reg = getReg(tok[2])) != -1)
			instr |= (reg << 16);
		// Parameter #3 - offset (Offset12)
		if ((reg = getReg(tok[3])) != -1)
			instr |= reg;
		else if (addrParam[1] && ((C = getConst(addrParam[1])) & CONST_12_ERR) == 0)
			instr |= C;
		else if (((C = getConst(tok[3])) & CONST_12_ERR) == 0)
			instr |= C;
	}

	// Branch type
	else if (type == BR) {
		instr |= 0x0A000000; // Set instruction format to 2 (BR type),
							 // and I to 1 (0000 1010 0000 ...)
		if (getOpCode(opcond[0]) == BL) {
			instr |= 0x01000000; // set L bit
		}
		// Get condition code (e.g. EQ = 0)
		if (opcond[1] && (cnd = getCond(opcond[1])) != INVALID_COND)
			instr |= cnd << 28;
		else
			instr |= 0xE0000000; // implicit "ALWAYS" condition

		// Get address value
		if (((A = getAddress(tok[1])) & CONST_24_ERR) == 0)
			instr |= A;
	}
	// Invalid Instruction type
	else {
		if (hasLabel) isJump = true; // do not increment PC!
		instr = UNDEFINED_INSTRUCTION;
	}
	if (DBG_ARMASSEMBLE) { printf("ARMAssemble:\tinstruction = "); printw(instr); }
	// Free opcond array
	if (opcond) {
		if (opcond[0]) free(opcond[0]);
		if (opcond[1]) free(opcond[1]);
	}
	// Return instruction
	return instr;
}

//
// Disassemble a 32-bit ARM instruction into
// a line of ARM assembly code.
// Returns a line of ARM assembly if successful,
// and NULL if it is not
// Note that the line must be freed by the user
// when it is no longer needed as it is dynamically allocated!
//
#define DBG_ARMDISASSEMBLE 0
char* ARMDisassemble(dword instr) {
	char *tokens[4] = {NULL}; 	// array of ARM instruction tokens (e.g. "ADD" "EQ" "r1" ...)
	char condstr[5] = {0}; 		// condition string
	char opstr[5] = {0}; 		// OpCode string
	char Rnstr[5] = {0}; 		// Rn register string
	char Rdstr[5] = {0}; 		// Rd (destination) register string
	char Operand2str[20] = {0};	// Operand2 string
	itype form = 0; 			// format
	opcode opval = 0; 			// OpCode value
	cond condval = 0; 			// Condition value
	int regval = 0; 			// register/constant value
	bool isImm = false; 		// Is Immediate operation flag (TRUE if Operand2 is a constant)
	int nTokens = 0; 			// number of tokens besides opcode and condition for instruction
	int j = 0;	// counter for parsing constants/addresses

	dword i = 0; // Iterator through a dword from leftmost to rightmost bit
	// Get the Condition field
	for (i = 0x80000000; i > 0x08000000; i >>= 1) {
		if (instr & i) {
			condval += (i >> 28);
		}
	}
	if (condval != 14) // "AL" (always) is implied
		strncpy(condstr, getCondStr(condval), 4); // Get condition string for the
												  // corresponding condition (e.g. "EQ")
	if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Condition Field;\n\tcondval = %d, \
condstr = %s\n", condval, condstr);

	// Get the Format
	for (;i > 0x02000000; i >>= 1) {
		if (instr & i) {
			form += (i >> 26);
		}
	}
	// Process each form
	switch (form) {
		// Data Processing Format
		case (DP):
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Instruction is DP type.\n");

			isImm = !!(instr & i); // set Immediate flag based on 'I' bit
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: isImmediate ? = %d\n", (int)isImm);
			i >>= 1; // Shift past the 'I' bit

			// Get the Opcode
			for (;i > 0x00100000; i >>= 1) {
				if (instr & i) {
					opval += (i >> 21);
				}
			}
			strncpy(opstr, getOpCodeStr(opval), 4); // Get opcode string for the
													// given opcode value (e.g. "ADD" = 14)
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Opcode;\n\topval = %d, \
opstr = %s\n", opval, opstr);

			i >>= 1; // Shift past the 'S' bit
					 // TODO: Implement suffix 'S' to instruction with this bit set
					 // the 'S' bit means that the instruction will change the condition flags

			bool cestCmp = isCmpInstr(opval);
			bool cestMov = isMovInstr(opval);
			// Get the Rn (first register) value
				// but not in Mov operations
			if (!cestMov) {
				for (regval = 0; i > 0x00008000; i >>= 1) {
					if (instr & i)
						regval += (i >> 16);
				}
				strncpy(Rnstr, getRegStr((byte)regval), 4);
				if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Rn Reg;\n\tregval = %d, \
	Rnstr = %s\n", regval, Rnstr);
			}

			// Get the Rd (destination reg) value
				// but not in cmp operations
			if (!cestCmp) {
				for (regval = 0; i > 0x00000800; i >>= 1) {
					if (instr & i)
						regval += (i >> 12);
				}
				strncpy(Rdstr, getRegStr((byte)regval), 4);
				if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Rd Reg;\n\tregval = %d, \
	Rdstr = %s\n", regval, Rdstr);
			}

			// Get the Operand2 (second register/constant) value
			for (regval = 0; i > 0; i >>= 1) {
				if (instr & i)
					regval += i;
			}
			if (isImm) {
				// Read as a constant
				j = 0;
				Operand2str[j++] = '#';
				if (regval & CONST_12_SIGN) {
					Operand2str[j++] = '-';
					regval = ((~regval) + 1) & 0x00000FFF; // 2's complement
				}
				sprintf(Operand2str+j, "%d", regval);
			}
			else {
				strncpy(Operand2str, getRegStr((byte)regval), 4);
			}
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Operand2;\n\tregval = %d, \
Operand2str = %s\n", regval, Operand2str);

			// Set tokens
			if (cestCmp) {
				nTokens = 2;
				tokens[0] = Rnstr;
				tokens[1] = Operand2str;
			}
			else if (cestMov) {
				nTokens = 2;
				tokens[0] = Rdstr;
				tokens[1] = Operand2str;
			}
			else {
				nTokens = 3;
				tokens[0] = Rdstr; // Rd
				tokens[1] = Rnstr; // Rn
				tokens[2] = Operand2str; // Operand2
			}

			break;
			// End Case 'DP'

		// Data Transfer Format
		case (DT):
			nTokens = 3;
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Instruction is DT type.\n");
			// Get the Opcode
			for (opval = 0;i > 0x00080000; i >>= 1) {
				if (instr & i) {
					opval += (i >> 20);
				}
			}
			strncpy(opstr, getOpCodeStr(opval), 4); // Get opcode string for the
													// given opcode value (e.g. "ADD" = 14)
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Opcode;\n\topval = %d, \
opstr = %s\n", opval, opstr);

			// Get the Rn (first register) value
			strcat(Rnstr, "[");
			for (regval = 0; i > 0x00008000; i >>= 1) {
				if (instr & i)
					regval += (i >> 16);
			}
			strncat(Rnstr, getRegStr((byte)regval), 3);
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Rn Reg;\n\tregval = %d, \
Rnstr = %s\n", regval, Rnstr);

			// Get the Rd (destination reg) value
			for (regval = 0; i > 0x00000800; i >>= 1) {
				if (instr & i)
					regval += (i >> 12);
			}
			strncat(Rdstr, getRegStr((byte)regval), 3);
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Get Rd Reg;\n\tregval = %d, \
Rdstr = %s\n", regval, Rdstr);

			// Get the Operand2 (second register/constant) value
			strcat(Operand2str, "#");
			for (regval = 0; i > 0; i >>= 1) {
				if (instr & i)
					regval += i;
			}
			j = 0;
			Operand2str[j++] = '#';
			if (regval & CONST_12_SIGN) {
				Operand2str[j++] = '-';
				regval = ((~regval) + 1) & 0x00000FFF; // 2's complement
			}
			sprintf(Operand2str+j, "%d", regval);
			strcat(Operand2str, "]");

			// Set tokens
			nTokens = 3;
			tokens[0] = Rdstr; // Rd
			tokens[1] = Rnstr; // Rn
			tokens[2] = Operand2str; // Offset12

			break;
			// End Case 'DT'

		// Branch Format
		case (BR):
			if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: Instruction is BR type.\n");
			// Operation string is just 'B'
			strncat(opstr, "B", 1);
			i >>= 1; // Shift past 'I' bit

			if (i & instr)
				strncat(opstr, "L", 1); // add 'L' if necessary
			i >>= 1;

			// Get the Address value
			for (regval = 0; i > 0; i >>= 1) {
				if (instr & i)
					regval += i;
			}
			j = 0;
			Operand2str[j++] = '#';
			if (regval & CONST_24_SIGN) {
				Operand2str[j++] = '-';
				regval = ((~regval) + 1) & 0x00FFFFFF; // 2's complement
			}
			sprintf(Operand2str+j, "%d", regval);

			// Set tokens
			nTokens = 1;
			tokens[0] = Operand2str; // Address

			break;
			// End Case 'BR'
	}
	// Combine the tokens into a line of ARM code
	if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: tokens;\n\t[0]=%s, [1]=%s, [2]=%s\n",
							Rnstr, Rdstr, Operand2str);

	char *line = malloc(sizeof(char)*(LINE_MAX+1)); // Allocate line
	memset(line, 0, sizeof(char)*(LINE_MAX+1)); // Clear memory
	strcat(line, opstr);	// Place OPERATION+CONDITION into the string "line"
	strcat(line, condstr);	// e.g. "ADD"+"EQ" => line = "ADDEQ"
	strcat(line, "\t");	// add some whitespace at the end
	if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: before strcombine;\n\tline = %s\n", line);
	strcombine(line, 80, tokens, nTokens, ", ", NULL);
						// Combine the rest of the tokens and add them to "line"
	if (DBG_ARMDISASSEMBLE) printf("ARMDisassemble: after strcombine;\n\tline = %s\n", line);
	// Return the ARM assembly line of code
	return line;
}

//
// Execute a 32-bit ARM instruction (in machine code)
// Changing registers and memory in the process
//
// Returns true (1) upon successful completion and false (0)
// if it fails, which may be the result of a failed condition!
// (e.g. ADDEQ and the EQ flag (Z) is not set)
//
extern ARM_DPFUNC DPFunctions[];
extern ARM_DTFUNC DTFunctions[];
#define DBG_ARMEXECUTEINSTR 0
bool ARMExecuteInstr(word instr) {
	itype form = 0; 			// format
	opcode opval = 0; 			// OpCode value
	cond condval = 0; 			// Condition value
	int reg1val = 0; 			// register1 value
	int regdval = 0;			// destination reg value
	int reg2val = 0;			// register2/Constrant value
	bool isImm = false; 		// Is Immediate operation flag (TRUE if Operand2 is a constant)

	word i = 0x80000000; // Iterator through a word from leftmost to rightmost bit

	// Get the Condition field
	for (;i > 0x08000000; i >>= 1) {
		if (instr & i) {
			condval += (i >> 28);
		}
	}
	if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: condition = %d\n", condval);

	// Check condition flags (VNCZ)
	if (!checkCondition(condval)) {
		if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Instruction not executed.\n");
		return false; // Do not execute
	}

	// Get the Format
	for (;i > 0x02000000; i >>= 1) {
		if (instr & i) {
			form += (i >> 26);
		}
	}
	// Process each form
	switch (form) {
		// Data Processing Format
		case (DP):
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Instruction is DP type.\n");

			isImm = (bool)!!(instr & i); // set Immediate flag based on 'I' bit
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: isImmediate ? = %d\n", (int)isImm);
			i >>= 1; // Shift past the 'I' bit

			// Get the Opcode
			for (;i > 0x00100000; i >>= 1) {
				if (instr & i) {
					opval += (i >> 21);
				}
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Opcode = %d\n", opval);

			i >>= 1; // Shift past the 'S' bit
					 // TODO:
					 // the 'S' bit means that the instruction will change the condition flags

			// Get the Rn (first register) value
			for (reg1val = 0; i > 0x00008000; i >>= 1) {
				if (instr & i)
					reg1val += (i >> 16);
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Rn = %d\n", reg1val);

			// Get the Rd (destination reg) value
			for (regdval = 0; i > 0x00000800; i >>= 1) {
				if (instr & i)
					regdval += (i >> 12);
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Rd = %d\n", regdval);

			// Get the Operand2 (second register/constant) value
			for (reg2val = 0; i > 0; i >>= 1) {
				if (instr & i)
					reg2val += i;
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Operand2 = %d (IsImm=%d)\n", reg2val, (int)isImm);

			// Execute the operation with the given parameters
			if (opval >= 0 && opval <= 15) {
				if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Executing operation %d.\n", opval);
				DPFunctions[opval-DP_OFFSET](regdval, reg1val, reg2val, isImm);
			}
			else {
				if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Error; Unknown operation %d.\n", opval);
				return false; // error; unknown operation
			}

			break;
			// End Case 'DP'

		// Data Transfer Format
		case (DT):
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Instruction is DT type.\n");

			// Get the Opcode
			for (;i > 0x00080000; i >>= 1) {
				if (instr & i) {
					opval += (i >> 20);
				}
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Opcode = %d\n", opval);

			// Get the Rn (first register) value
			for (reg1val = 0; i > 0x00008000; i >>= 1) {
				if (instr & i)
					reg1val += (i >> 16);
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Rn = %d\n", reg1val);

			// Get the Rd (destination reg) value
			for (regdval = 0; i > 0x00000800; i >>= 1) {
				if (instr & i)
					regdval += (i >> 12);
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Rd = %d\n", regdval);

			// Get the Operand2 (second register/constant) value
			for (reg2val = 0; i > 0; i >>= 1) {
				if (instr & i)
					reg2val += i;
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Offset12 = %d\n", reg2val);

			// Execute the operation with the given parameters
			if (opval >= 24 && opval <= 26) {
				if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Executing operation %d.\n", opval);
				DTFunctions[opval-DT_OFFSET](regdval, reg1val, reg2val);
			}
			else {
				if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Error; Unknown operation %d.\n", opval);
				return false; // error; unknown operation
			}

			break;
			// End Case 'DT'

		// Branch Format
		case (BR):
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Instruction is BR type.\n");
			bool isLink = false;
			i >>= 1; // Shift past 'I' bit
			if (i & instr)
				isLink = true;
			i >>= 1;
			// Get the Address value
			for (reg2val = 0; i > 0; i >>= 1) {
				if (instr & i)
					reg2val += i;
			}
			if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Address24 = %d\n", reg2val);

			if (isLink) {
				if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Executing BL operation.\n");
				ARM_BL(reg2val);
			}
			else {
				if (DBG_ARMEXECUTEINSTR) printf("ARMExecuteInstr: Executing B operation.\n");
				ARM_B(reg2val);
			}
			isJump = true;

			break;
			// End Case 'BR'
	}
	// Finished
	return true;
}

// Load a file by performing the following operations:
//
// 1)	Split up the file into pages (ea. = 1KB)
// 2)	Load a "Page Table" which references all page locations (4GB/1KB=4194304 pages max.)
// 3)	Load the first 4 pages into main memory (memory = 4KB)
//
extern word page_table[];
#define DBG_ARMLOADFILE 0
bool ARMLoadFile(FILE *fpin) {
	regs[PC] = 0;
	word elemsRead = 0;
	if (fpin == NULL)
		return false; // error
	// Read file's data
	word buf[M1KB/4];
	// Make Page Table
	for (dword i=0; i<M4GB/M1KB; i++) {
		memset(buf, 0, M1KB/4);
		// Read a page of data (1KB)
		elemsRead = fread(&buf, sizeof(word), sizeof(buf), fpin);
		// No data is read - stop
		if (elemsRead == 0) { 
			break;
		}
		nInstructions += elemsRead;
		// For the first 4 pages of memory, add them to the main memory
		if (i < 4) {
			memcpy(&memory[i*(M1KB/4)], buf, M1KB/4);
			page_table[i] |= VALIDB;
		}
		// Generate page file name
		char fname[27] = {0}, pnum[20] = {0};
		itoa(i, pnum);
		strncat(fname, "PG/Mem", 7);
		strncat(fname, pnum, 20);
		// Write data to page file
		FILE *opage = fopen(fname, "wb");
		fwrite(buf, M1KB, 1, opage);
		// Close page file
		fclose(opage);
		// Update Page Table
		page_table[i] |= i & PHYS_PAGE_NUMBER; // Set physical location = i
		page_table[i] |= REFB; // REFB=1 means page exists
	}
	// Done
	return true;
}

// Check condition 'c' against the condition flags VNCZ
// Returns true if the condition is met and false otherwise
bool checkCondition(cond c) {
	switch (c) {
		case EQ:
			return flags[ZF];
		case NE:
			return !flags[ZF];
		case HS:
			return flags[CF];
		case LO:
			return !flags[CF];
		case MI:
			return flags[NF];
		case PL:
			return !flags[NF];
		case VS:
			return flags[VF];
		case VC:
			return !flags[VF];
		case HI:
			return flags[CF] && !flags[ZF];
		case LS:
			return !flags[CF] && flags[ZF];
		case GE:
			return flags[NF] == flags[VF];
		case LT:
			return !flags[ZF] && (flags[NF] != flags[VF]);
		case GT:
			return !flags[ZF] && (flags[NF] == flags[VF]);
		case LE:
			return flags[ZF] || (flags[NF] != flags[VF]);
		case AL:
			return true;
		case NV:
		default:
			return false;
	}
}

// Get the number value of a register stored in the
// string "reg"
// Returns -1 upon failure or if the string is a constant,
// and the register number (from 0 to 15) upon success.
#define DBG_GETREG 0
regn getReg(char *reg) {
	int regValue = -1;
	int regIDX = 0;
	if (reg == NULL || reg[0] == '#') {
		if (DBG_GETREG) printf("getReg:\tError: reg is not defined or is a constant.\n");
		return -1; // Failure; also occurs if the token is a *constant*
				   // rather than a register
	}
	reg[0] = toupper(reg[0]); // ignore case on single letter register names
	// Get the reg's value
	if (reg[0] == 'A' && isdigit(reg[1]) && reg[2] == '\0') {
		regIDX = atoi(reg+1);
		if (regIDX >= 1 && regIDX <= 4) {
			regValue = A1+regIDX+REG_A1_OFFSET;
		}
	}
	else if (reg[0] == 'V' && isdigit(reg[1]) && reg[2] == '\0') {
		regIDX = atoi(reg+1);
		if (regIDX >= 1 && regIDX <= 8) {
			regValue = V1+regIDX+REG_V1_OFFSET;
		}
	}
	else if (reg[0] == 'R') {
		regIDX = atoi(reg+1);
		if (regIDX >= 0 && regIDX <= 15) {
			regValue = regIDX;
		}
	}
	else if (strcasecmp(reg, "IP") == 0)
		regValue = 12;
	else if (strcasecmp(reg, "SP") == 0)
		regValue = 13;
	else if (strcasecmp(reg, "LR") == 0)
		regValue = 14;
	else if (strcasecmp(reg, "PC") == 0)
		regValue = 15;

	// Check if the register number exceeds 15 or is lower than 0
	if (regValue < 0 || regValue > 15) {
		if (DBG_GETREG) printf("getReg:\tError: reg exceeds boundries. (=%d)\n", regValue);
	}
	return (regn)regValue;
}

// Get the Register's string based on the number supplied
// Returns the register's name ("r1","r2",etc.) on success,
// or the NULL-string on failure ("\0")
#define DBG_GETREGSTR 0
const char* getRegStr(regn regnum) {
	if (regnum >= sizeof(RegStr)/sizeof(RegStr[0])) {
		return NULL;
	}
	return RegStr[regnum];
}

// Get the condition of an ARM instruction,
// which is appended to the instruction (e.g. ADDEQ means
// "add if equal").
//
// Returns the condition's code if successful, and INVALID_COND (-1)
// otherwise.
//
//extern char *CondStr[];
#define DBG_GETCOND 0
cond getCond(char *instr) {
	if (instr == NULL)
		return INVALID_COND; // error; null instruction
	cond c = INVALID_COND;
	if (instr != NULL) {
		strupper(instr);
		for (int i=0; i<(sizeof(CondStr)/sizeof(CondStr[0])); i++) {
			if (strcmp(instr, CondStr[i]) == 0) {
				c = (cond)i;
				break;
			}
		}
	}
	return c;
}


// Get the string for the condition given the
// condition number supplied
// Returns the string for the condition ("EQ", "NV", "NE", etc.)
// or NULL if there's an error
//extern char *CondStr[];
#define DBG_GETCONDSTR 0
const char* getCondStr(cond c) {
	if (c < 0 || c > sizeof(CondStr)/sizeof(CondStr[0]))
		return NULL; // condition doesn't exist
	// Return the condition's string
	return CondStr[c];
}

//
// Get the constant value of an ARM parameter
// which has the format '#XX' (where XX is a number)
//
// Returns a 16-bit hword with:
//
// The value specified as long as |N| <= 2^11,
// with *2's complement* being used for negative numbers,
// and the first 4 bits set to 0000,
//
// An "overflow" value with the first 4 bits
// set to 0001; this means the value is too large,
//
// An "underflow" value with the first 4 bits
// set to 0010; this means the value is too large, or
//
// A "NaN" value with the first 4 bits
// set to 0100; this means the value is invalid
//
#define DBG_GETCONST 0
hword getConst(char *C) {
	if (C == NULL || C[0] != '#') {
		if (DBG_GETCONST) printf("getConst:\tConstant is NaN.\n");
		return CONST_12_NAN;
	}

	bool isNegative = false;
	int i = 1;
	int tmp = 0;
	int base = 10;
	// Check for negative sign
	if (C[i] == '-') {
		i++;
		isNegative = true;
		if (DBG_GETCONST) printf("getConst:\tConstant is negative.\n");
	}
	// Check for base (decimal; default, hex, octal, binary)
	if (C[i] == '0' && tolower(C[i+1]) == 'x') {
		// Hex
		i += 2;
		base = 16;
	}
	else if (C[i] == '0' && tolower(C[i+1]) == 'b') {
		// Binary
		i += 2;
		base = 2;
	}
	else if (C[i] == '0') {
		// Octal
		i++;
		base = 8;
	}
	// Get constant's value as an int
	while (C[i] != '\0') {
		C[i] = toupper(C[i]);
		tmp *= base;
		tmp += C[i]+(C[i] > '9' ? 10-'A':-'0');
		if (DBG_GETCONST) printf("getConst:\ttmp = %d\n", tmp);
		i++;
	}
	if (isNegative) tmp *= -1; // Apply negative sign to int value
	// Check for overflow/underflow
	if (tmp > 0x000007FF) {
		// Overflow
		if (DBG_GETCONST) printf("getConst:\tConstant has overflowed.\n");
		return CONST_12_OVRF;
	}
	else if (tmp < -0x000007FF) {
		// Underflow
		if (DBG_GETCONST) printf("getConst:\tConstant has underflowed.\n");
		return CONST_12_UDRF;
	}
	if (isNegative) {
		tmp *= -1; // Revert to absolute value
		tmp = ((~tmp) + 1) & 0x00000FFF; // 2's complement and mask last 12 bits
	}
	// Return the constant as a hword (16-bit)
	if (DBG_GETCONST) { printf("getConst:\tC = \n"); printw(tmp); }
	return tmp;
}

//
// Get the address (constant) of an ARM parameter
// which has the format '#XX' (where XX is a number) OR is a LABEL,
// such as 'LOOP'
//
// Returns a 32-bit word with:
//
// The value specified as long as |N| <= 2^23,
// with *2's complement* being used for negative numbers,
// and the first 4 bits set to 0000,
//
// An "overflow" value with the first 4 bits
// set to 0001; this means the value is too large,
//
// An "underflow" value with the first 4 bits
// set to 0010; this means the value is too large, or
//
// A "NaN" value with the first 4 bits
// set to 0100; this means the value is invalid
//
// *** NOTE: *** If the address is a label which has not been resolved
// (i.e. has not yet appeared before in the code), then this function
// will return 0 BUT will place the 'unresolved instruction' in the
// symbol table for later resolution.
//
#define DBG_GETADDRESS 0
word getAddress(char *A) {
	if (A == NULL) {
		if (DBG_GETADDRESS) printf("getAddress:\tAddress is NaN.\n");
		return CONST_24_NAN;
	}
	bool isNegative = false;
	int addrval = 0;
	// Is the address a constant or a label?
	if (!isdigit(A[0])) {
		// Address is a label
		// First check if it exists in the symbol table
		HEntry *entry = hfind(symbolTable, A);
		if (entry) {
			// Symbol was found
			// Check if it is resolved; if it is,
			// access its address value
			if (entry->isResolved) {
				addrval = (entry->address - regs[PC]) - 1;
				if (addrval < 0)
					isNegative = true;
			}
			// Otherwise, add a new unresolved node to the list for that entry
			else {
				word *addr = malloc(sizeof(word));
				*addr = regs[PC]; // copy current PC to symbol's address
				llnode *node = createLLNode((void*)addr);
				lladdHead(entry->unresolvedInstr, node);
			}
		}
		else {
			// Symbol not found
			// add it to the hash table (it must be resolved)
			if (symbolTable == NULL) { // if table doesn't exist, create it
				symbolTable = createHTable(DEFAULT_TABLE_SIZE);
			}
			HEntry *entry = createHEntry(A, 0, false);
			// add this instruction to symbol's list of unresolved instructions
			word *addr = malloc(sizeof(word));
			*addr = regs[PC]; // copy current PC to symbol's address
			llnode *node = createLLNode((void*)addr);
			lladdHead(entry->unresolvedInstr, node);
			// insert symbol into symbol table
			hinsert(symbolTable, entry, true);
		}
	}
	else {
		int i = 1;
		int base = 10;
		// Check for negative sign
		if (A[1] == '-') {
			i++;
			isNegative = true;
			if (DBG_GETADDRESS) printf("getAddress:\tAddress is negative.\n");
		}
		// Check for base (decimal; default, hex, octal, binary)
		if (A[i] == '0' && toupper(A[i+1]) == 'x') {
			// Hex
			i += 2;
			base = 16;
		}
		else if (A[i] == '0' && toupper(A[i+1]) == 'b') {
			// Binary
			i += 2;
			base = 2;
		}
		else if (A[i] == '0') {
			// Octal
			i++;
			base = 8;
		}
		// Get address' value as an int
		while (A[i] != '\0') {
			A[i] = toupper(A[i]);
			addrval *= base;
			addrval += A[i]+(A[i] > '9' ? 10-'A':-'0');
			if (DBG_GETADDRESS) printf("getAddress:\ttmp = %d\n", addrval);
			i++;
		}
		if (isNegative) addrval *= -1; // Apply negative sign to int value
		// Check for overflow/underflow
		if (addrval > 0x007FFFFF) {
			// Overflow
			if (DBG_GETADDRESS) printf("getAddress:\tAddress has overflowed.\n");
			return CONST_24_OVRF;
		}
		else if (addrval < -0x007FFFFF) {
			// Underflow
			if (DBG_GETADDRESS) printf("getAddress:\tAddress has underflowed.\n");
			return CONST_24_UDRF;
		}
	}
	if (isNegative) {
		addrval *= -1; // Revert to absolute value
		addrval = ((~addrval) + 1) & 0x00FFFFFF; // 2's complement and mask last 24 bits
	}
	// Return the constant as a word (32-bit)
	if (DBG_GETADDRESS) { printf("getAddress:\tA = \n"); printw(addrval); }
	return addrval;
}

// Get the Opcode of an ARM instruction specified
// in the string (token) "op"
// Returns NOP (0) if the opcode doesn't exist, or the
// opcode's value otherwise
//extern char *OpCodeStr[];
#define DBG_GETOPCODE 0
opcode getOpCode(char *op) {
	opcode opc = INVALID_COND;
	if (op != NULL) {
		strupper(op);
		for (int i=0; i<(sizeof(OpCodeStr)/sizeof(OpCodeStr[0])); i++) {
			if (strcasecmp(op, OpCodeStr[i]) == 0) {
				opc = (opcode)i;
				break;
			}
		}
	}
	return opc;
}

// Get the string for the operation given the
// opcode number supplied
// Returns the string for the operation ("ADD", "SUB", "B", etc.)
// or NULL if there's an error
#define DBG_GETOPCODESTR 0
const char* getOpCodeStr(opcode opc) {
	if (opc == B)
		return "B"; // Because 'B' isn't in the opcode table
	else if (opc < 0 || opc > sizeof(OpCodeStr)/sizeof(OpCodeStr[0]))
		return NULL; // opcode doesn't exist
	// Return the opcode's string
	return OpCodeStr[opc];
}

// Returns:
// 	1 if the type is DP (Data Processing),
// 	2 if the type is DT (Data Transfer),
// 	0 if it is an invalid ARM instruction type
#define DBG_GETITYPE 0
itype getIType(char *instr) {
	strupper(instr);
	for (int i = 0; i < sizeof(OpCodeStr)/sizeof(OpCodeStr[0]); i++) {
		if (i <= 15 && strncasecmp(OpCodeStr[i], instr, 3) == 0)
			return DP;
		else if (i > 15 && i <= 27 && strncasecmp(OpCodeStr[i], instr, 3) == 0)
			return DT;
		else if (i > 27 && i <= 29 && strncasecmp(OpCodeStr[i], instr, 3) == 0)
			return BR;
	}
	return INVALID_ITYPE;
}

// Check if operation is mov or cmp type
bool isCmpInstr(opcode op) {
	if (op == TEQ || op == TST || op == CMP || op == CMN)
		return true;
	else return false;
}
bool isMovInstr(opcode op) {
	if (op == MOV || op == MVN)
		return true;
	else return false;
}

//
// Tokenize string into array of strings (tokens);
//
// Tokens are seperated by all symbols in "sep",
// the occurence of any symbol in "delim" will terminate the tokenization,
// and anything within each duo of symbols in "grp" is treated as ONE token;
// ex: grp = "[]()<>" means that '[' and ']' create a group, '(' and ')', etc.
//
// If "isCreate" is true (1), then a default token array will be created.
//
// Returns NULL if there's an error, and an array of strings (tokens) upon success.
//
#define DBG_TOKENIZE 0
char** tokenize(char *str, char *sep, char *delim, char *grp, bool isCreate) {
	char **tok = NULL;
	if (DBG_TOKENIZE) printf("tokenize:\tAllocating default token array...\n");

	// Create token array
	if (isCreate) {
		tok = malloc(sizeof(char*)*DEFAULT_NUMBER_OF_TOKENS);
		memset(tok, 0, sizeof(char*)*DEFAULT_NUMBER_OF_TOKENS);

		// Create each token's string
		if (tok != NULL) {
			if (DBG_TOKENIZE) printf("tokenize:\tAllocating default token slots...\n");
			for (int i=0; i<DEFAULT_NUMBER_OF_TOKENS; i++) {
				if (DBG_TOKENIZE) printf("\t\t\tToken #%d added.\n", i);
				tok[i] = malloc(sizeof(char)*DEFAULT_TOKEN_LENGTH);
				memset(tok[i], 0, sizeof(char)*DEFAULT_TOKEN_LENGTH);
			}
		}
	}

	// Check for invalid parameters or insufficient memory
	hword len = strlen(str);
	if (DBG_TOKENIZE) printf("tokenize:\tString length = %u\n", len);
	if (len <= 0 || str == NULL || tok == NULL || *tok == NULL) {
		if (DBG_TOKENIZE) printf("tokenize:\tTokenize has failed.\n");
		// Free any allocated memory
		for (int i=0; i < DEFAULT_NUMBER_OF_TOKENS && tok != NULL; i++)
			if (tok[i] != NULL) free(tok[i]);
		if (tok != NULL) free(tok);
		// Exit and return failure (NULL)
		return NULL; // Failure
	}

	// Tokenize string
	bool isBracket = false;

	int j = 0, k = 0;
	for (int i=0; i < len && str[i] != '\0' && !index(delim, str[i]); i++) {
		if (DBG_TOKENIZE) printf("tokenize:\ti = %d, j = %d, k = %d\n", i, j, k);
		if (index(grp, str[i])) {
			isBracket = !isBracket; // TODO: use a stack to match open/close brackets
									// For now, we just check if the symbol is a "bracket"
			continue;
		}
		if (isBracket || !index(sep, str[i])) {
			tok[j][k++] = str[i];
		}
		else if (k > 0) { // Add only if there's actually something in the token
			tok[j][k] = '\0';
			if (DBG_TOKENIZE) printf("\t\t\tToken \"%s\" added at #%d.\n", tok[j], j);
			j += 1;
			k = 0;
		}
		// Otherwise, ignore the symbol
	}
	tok[j][k] = '\0';

	// Exit
	if (DBG_TOKENIZE) printf("tokenize:\tTokenize has succeeded.\n");
	return tok; // Success
}

// Convert entire string to upper case,
// OVERWRITING the original string
char* strupper(char *str) {
	int j = strlen(str);
	for (int i=0; i<j; i++) {
		str[i] = toupper(str[i]);
	}
	return str;
}

//
// Split string into 2 parts at the index specified;
// Returns {str, NULL} if there is an error or if the index is
// not less than the length of the string,
// and {first str, second str} otherwise.
//
#define DBG_STRISPLIT 0
char** strnsplit(char **dest, char *src, unsigned i) {
	unsigned len = strlen(src); // Get length of string
	// If the index exceeds that of
	// the string to check, then return {original string, NULL}
	if ((i+1) >= len) {
		dest[0] = src;
		dest[1] = NULL;
		if (DBG_STRISPLIT) printf("srcisplit:\tsrc2 = {%s, NULL}\n", dest[0]);
		return dest;
	}
	dest[0] = calloc(sizeof(char)*(i+1), sizeof(char));
	memset(dest[0], 0, sizeof(char)*(i+1));
	dest[1] = calloc(sizeof(char)*(len-i-1), sizeof(char));
	memset(dest[1], 0, sizeof(char)*(len-i-1));
	// Split the string
	for (int j=0; j<len; j++) {
		if (j < i) {
			dest[0][j] = src[j];
		} else {
			dest[1][j-i] = src[j];
		}
	}
	if (DBG_STRISPLIT) printf("srcisplit:\tsrc2 = {%s, %s}\n", dest[0], dest[1]);
	for (int j=len-i; j<len; j++)
		dest[1][j] = '\0'; // TODO: why does random garbage get shoved into dest[1] sometimes???
	return dest;
}

//
// Combine a series of tokens onto the end of a given
// string, seperated by 'seperators' and ended with
// 'delim' + '\0'
//
// Concatenates n strings in 'tok'
// to the end of the string 'src' seperated by 'sep' and
// ended with 'delim' + '\0'.
// If sep is NULL, then there are no seperations between strings.
// If delim is NULL, then only '\0' is appended afterwards.
//
//
// Returns the combined string upon success.
// If there is an error, then function returns the unchanged string.
// An error will occur if dest or tok is NULL, if n is 0 or negative,
// *or* if the maximum size of src is ever exceeded.
// Size of dest is given by parameter 'size'.
//
#define DBG_STRCOMBINE 0
char* strcombine(char *dest, int size,
				char **tok, int n,
				const char *sep, const char *delim) {
	if (!dest || !tok || n <= 0) {
		if (DBG_STRCOMBINE) printf("strcombine: Error; NULL or invalid parameter.\n");
		return dest; // Error
	}
	int len = strlen(dest);
	char *cpy = malloc(sizeof(char)*(size+1));
	memset(cpy, 0, sizeof(char)*(size+1));
	strncpy(cpy, dest, len);
	for (int i=0; i<n; i++) {
		if (tok[i]) {
			if (strlen(tok[i])+len > size) { // Size of the array is exceeded
				free(cpy);
				return dest; // Error
			}
			strcat(cpy, tok[i]);
			if (sep && i != n-1) {
				if (strlen(sep)+strlen(cpy) > size) // size of seperator + cpy's size exceeds maximum size
					{ if (DBG_STRCOMBINE) printf("strcombine: Error; String size exceeded.\n"); }
				else strcat(cpy, sep);
				// TODO: Must check if there are any more tokens to concat,
				// because when we add the last one we shouldn't put the seperator
				// at the end (e.g. "ADD r1, r2, r3, " should be "ADD r1, r2, r3"!
			}
		}
	}
	if (delim) strcat(dest, delim);
	if (DBG_STRCOMBINE) printf("strcombine: cpy = %s\n", cpy);
	strncpy(dest, cpy, size); // copy string back
	free(cpy);
	return dest;
}

// Returns the reverse of a given string
// but does not change the original string
// Returns NULL if there's an error, and the reversed
// string otherwise
#define DBG_STRREV 0
char* strrev(char *str) {
	if (str == NULL)
		return NULL; // error; string is null
	int len = strlen(str);
	char *rstr = malloc(sizeof(char)*len);
	memset(rstr, 0, len);
	for (int i=0; i<len; i++) {
		rstr[i] = str[len-i];
	}
}

// Print a word as a binary string
#define DBG_PRINTDW 0
void printw(word n){
	bool isSpace = false;
	int j = 1;
	for(word i = 0x80000000; i > 0; i = i >> 1){
		printf("%d", (int)!!(n & i));
		if (j++ == 4) {
			printf(" ");
			j = 1;
		}
	}
	printf("\n");
}
