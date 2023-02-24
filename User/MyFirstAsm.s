	AREA RESET,DATA, READONLY
	EXPORT __Vectors
	DCD 0x20002000
	DCD Reset_Handler
__Vectors
	AREA	AFTER_RESET, 	CODE
	EXPORT	Reset_Handler
Reset_Handler
	mov r8, #-5
loop
	ADDS r8, r8, #5
	b loop
	END