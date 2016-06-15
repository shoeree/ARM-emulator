; Test file for the ARM Virtual Machine
; Sterling Hoeree
; 2011/04/26

; BEGIN

; MAIN
MAIN:
	MOV R0, #0xA ; R0 = 10
	MOV R1, #010 ; R1 = 8
	ADD R2, R0, R1 ; R2 = R0 + R1 = 10 + 8 = 18
	; While R2 > 0
	WHILE_R2_GT_0:
		SUB R2, R2, #0b110 ; R2 = R2 - 6
		CMP R2, #0 ; Compare R2 & 0
		BGT WHILE_R2_GT_0
	; End While
	MOV R8, #0   ; R8 = 0
	
	MOV R0, #100 ; R0 = 100
	BL EX_FUNCTION
	STR R0, [R8, #0] ; Store R0 at R8[0]
	
	MOV R0, #40  ; R1 = 40
	BL EX_FUNCTION
	STR R0, [R8, #4] ; Store R0 at R8[1]
	
	MOV R0, #50  ; R2 = 50
	BL EX_FUNCTION
	STR R0, [R8, #8] ; Store R0 at R8[2]
	
	; Restore the values to registers r0, r1, r2
	LDR R0, [R8, #0]
	LDR R1, [R8, #4]
	LDR R2, [R8, #8]
	
	B END ; Jump to END
; END MAIN

; EX_FUNCTION
EX_FUNCTION:
	CMP R0, #50 ; Compare R0 & 50
	ADDLT R0, R0, #50 ; R0 = R0 + 50 if R0 < 50
	SUBGT R0, R0, #50 ; R0 = R0 - 50 if R0 > 50
	MOV PC, LR ; Return
; END EX_FUNCTION

; END
END: