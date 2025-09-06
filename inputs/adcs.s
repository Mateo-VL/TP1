.text
MOVZ X0, #0xFFFF     // X0 = 0xFFFF
LSr X1, X0, #0x10
ADDS X1, X1, X0
LSL X1, X1, #0x10
ADDS X1, X1, X0
LSL X1, X1, #0x10
ADDS X1, X1, X0
LSL X1, X1, #0x10
ADDS X1, X1, X0
MOVZ X0, #1
ADDS X2, X1, X0
ADCS X3, X4, X5

MOVZ X6, #5          // X6 = 5
MOVZ X7, #3          // X7 = 3
ADDS X8, X6, X7      // 5 + 3 = 8 (sin carry)
ADCS X9, X6, X7      // X9 = 5 + 3 + 0 = 8

// Finalizar
HLT 0
