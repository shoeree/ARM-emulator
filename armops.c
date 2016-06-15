/* Project "ARMed" Source File
 * Contains definitions of all operations
 * supported by this ARMVM (e.g. ADD, STR, etc.)
 * as well as function pointer arrays to those operations
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/04/17
 * Date Modified:2011/06/28
 *                 
                                         */
#include <stdlib.h>
#include <stdio.h>
#include "armops.h"
#include "vmem.h"

extern regn regs[];
extern bool flags[];
extern word memory[];
extern word stackMem[];

// Debug Mode
#define DBG_ARMOPS 0

// Function Definitions
// Data Processing
void ARM_AND(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] & (isImm ? Operand2 : regs[Operand2]);
}

void ARM_EOR(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] ^ (isImm ? Operand2 : regs[Operand2]);
}

void ARM_SUB(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] - (isImm ? Operand2 : regs[Operand2]);
}

void ARM_RSB(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = (isImm ? Operand2 : regs[Operand2]) - regs[Rn];
}

void ARM_ADD(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] + (isImm ? Operand2 : regs[Operand2]);
}

void ARM_ADC(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] + (isImm ? Operand2 : regs[Operand2]) + flags[CF];
}

void ARM_SBC(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] - (isImm ? Operand2 : regs[Operand2]) - flags[CF];
}

void ARM_RSC(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = (isImm ? Operand2 : regs[Operand2]) - regs[Rn] - flags[CF];
}

void ARM_TST(regn Rd, regn Rn, word Operand2, bool isImm) {
	// does not use Rd
	word tmp = regs[Rn] & (isImm ? Operand2 : regs[Operand2]);
	if (tmp == 0) flags[ZF] = 1;
	else flags[ZF] = 0;
}

void ARM_TEQ(regn Rd, regn Rn, word Operand2, bool isImm) {
	// does not use Rd
	word tmp = regs[Rn] ^ (isImm ? Operand2 : regs[Operand2]);
	if (tmp & 0x80000000) {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = true;
		flags[ZF] = false;
	} else if (tmp == 0) {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = false;
		flags[ZF] = true;
	}
	else {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = false;
		flags[ZF] = false;
	}
}

void ARM_CMP(regn Rd, regn Rn, word Operand2, bool isImm) {
	// does not use Rd
	int tmp = regs[Rn] - (isImm ? Operand2 : regs[Operand2]);
	if (tmp < 0) {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = true;
		flags[ZF] = false;
	} else if (tmp == 0) {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = false;
		flags[ZF] = true;
	}
	else {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = false;
		flags[ZF] = false;
	}
}

void ARM_CMN(regn Rd, regn Rn, word Operand2, bool isImm) {
	// does not use Rd
	dword tmp = regs[Rn] + (isImm ? Operand2 : regs[Operand2]);
	if (tmp > 0xFFFFFFFF) {
		flags[VF] = true;
		flags[CF] = false;
		flags[NF] = false;
		flags[ZF] = false;
	}
	else if (tmp < 0) {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = true;
		flags[ZF] = false;
	} else if (tmp == 0) {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = false;
		flags[ZF] = true;
	}
	else {
		flags[VF] = false;
		flags[CF] = false;
		flags[NF] = false;
		flags[ZF] = false;
	}
}

void ARM_ORR(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] | (isImm ? Operand2 : regs[Operand2]);
}

void ARM_MOV(regn Rd, regn Rn, word Operand2, bool isImm) {
	// does not use Rn
	regs[Rd] = (isImm ? Operand2 : regs[Operand2]);
}

void ARM_BIC(regn Rd, regn Rn, word Operand2, bool isImm) {
	regs[Rd] = regs[Rn] & (~(isImm ? Operand2 : regs[Operand2]));
}

void ARM_MVN(regn Rd, regn Rn, word Operand2, bool isImm) {
	// does not use Rn
	regs[Rd] = ~(isImm ? Operand2 : regs[Operand2]);
}

// Data Transfer
void ARM_LDR(regn Rd, regn Rn, word Offset12) {
	// Stack
	if (Rn == SP) {
		regs[Rd] = *((char*)(stackMem+regs[Rn]+Offset12));
	}
	// Data
	else {
		word buf = 0;
		while (!(buf = getMemory(regs[Rn]+Offset12)));
		regs[Rd] = buf;
	}
}

void ARM_STR(regn Rd, regn Rn, word Offset12) {
	// Stack
	if (Rn == SP) {
		*((char*)(stackMem+regs[Rn]+Offset12)) = regs[Rd];
	}
	// Data
	else {
		bool isSet = false;
		while (!(isSet = setMemory(regs[Rn]+Offset12, regs[Rd])));
	}
}

// Branch
void ARM_B (word Address) {
	if (Address & CONST_24_SIGN) {
		Address = ((~Address)+1) & 0x00FFFFFF;
		regs[PC] -= Address;
	}
	else {
		regs[PC] += Address;
	}
}

void ARM_BL (word Address) {
	regs[LR] = regs[PC];
	if (Address & CONST_24_SIGN) {
		Address = ((~Address)+1) & 0x00FFFFFF;
		regs[PC] -= Address;
	}
	else {
		regs[PC] += Address;
	}
}
