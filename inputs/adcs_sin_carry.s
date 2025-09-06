.terunxt

MOVZ X0, #0          // X0 = 0
MOVZ X1, #5          // X1 = 5
MOVZ X2, #3          // X2 = 3
ADCS X3, X1, X2      // X3 = 5 + 3 + 0 = 8, flags: N=0, Z=0, C=0, V=0

HLT 0
