/* Project "ARMed" Header File
 * Contains prototypes of all operations
 * supported by the ARM architecture (e.g. ADD, STR, etc.)
 * 
 * Author: 	Sterling Hoeree
 * ID:		3090300043 ZJU-SFU
 * 
 * Date Created: 2011/04/17
 * Date Modified:2011/04/26
 *                                                          */
#include "armdefs.def"
// Note: 
// DP operations take 4 parameters (reg, reg, operand2, isImm flag)
// DT operations take 3 parameters (reg, reg, offset12)
// B (branch) instructions only take 1 parameter (address)
 
// Prototypes
// Data Processing
void	ARM_AND(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_EOR(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_SUB(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_RSB(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_ADD(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_ADC(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_SBC(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_RSC(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_TST(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_TEQ(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_CMP(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_CMN(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_ORR(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_MOV(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_BIC(regn Rd, regn Rn, word Operand2, bool isImm);
void	ARM_MVN(regn Rd, regn Rn, word Operand2, bool isImm);
// Data Transfer
void	ARM_LDR(regn Rd, regn Rn, word Offset12);
void	ARM_STR(regn Rd, regn Rn, word Offset12);
// Branch
void	ARM_B  (word Address);
void	ARM_BL (word Address);
