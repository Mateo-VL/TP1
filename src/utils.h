#include "shell.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const char *instruction_names[] = {
    "B", "BEQ", "BNE", "BGT", "BLT", "BGE", "BLE",
    "HLT", "ADDSer", "ADDSim", "SUBSer", "SUBSim", "CMPer", "CMPim",
    "ANDS", "EOR", "ORR", "BR", "LSL", "LSR", "STUR",
    "STURB", "STURH", "LDUR", "LDURB", "LDURH", "MOVZ", "ISNOT",
    "ADDim", "ADDer", "MUL", "CBZ", "CBNZ"};

typedef enum
{
  B = 0,
  BEQ = 1,
  BNE = 2,
  BGT = 3,
  BLT = 4,
  BGE = 5,
  BLE = 6,
  HLT = 7,
  ADDSer = 8,
  ADDSim = 9,
  SUBSer = 10,
  SUBSim = 11,
  CMPer = 12,
  CMPim = 13,
  ANDS = 14,
  EOR = 15,
  ORR = 16,
  BR = 17,
  LSL = 18,
  LSR = 19,
  STUR = 20,
  STURB = 21,
  STURH = 22,
  LDUR = 23,
  LDURB = 24,
  LDURH = 25,
  MOVZ = 26,
  ISNOT = 27,
  ADDim = 28,
  ADDer = 29,
  MUL = 30,
  CBZ = 31,
  CBNZ = 32,
  INVALID_INSTRUCTION = -1 // To handle invalid cases
} Instruction;

typedef struct
{
  uint64_t result;
  int flagN;
  int flagZ;
} AddWithCarryResult;

uint32_t instructions[] = {
    0b10101011001000000000000000000000, // instruction1 (AB)
    0b10110001000000000000000000000000, // instruction2 (B1)
    0b11101011001000000000000000000000, // instruction3 (CB)
    0b11110001000000000000000000000000, // instruction4 (D1)
    0b11010100010000000000000000000000, // instruction5 (D4 Halt)
    0b11101011001000000000000000011111, // instruction6 (EB)
    0b11110001000000000000000000011111, // instruction7 (F1)
    0b11101010000000000000000000000000, // instruction8 (EA)
    0b11001010000000000000000000000000, // instruction9 (CA)
    0b10101010000000000000000000000000, // instruction10 (AA)
    0b00010100000000000000000000000000, // instruction11 (14)
    0b11010110000111110000000000000000, // instruction12 (D6)
    0b01010100000000000000000000000000, // instruction13 (54 B.cond)
    0b01010100000000000000000000000001, // instruction14
    0b01010100000000000000000000001100, // instruction15
    0b01010100000000000000000000001011, // instruction16
    0b01010100000000000000000000001010, // instruction17
    0b01010100000000000000000000001101, // instruction18
    0b11010011000000000111110000000000, // instruction19 (D3)
    0b11010011000000001111110000000000, // instruction20 (D3)
    0b11111000000000000000000000000000, // instruction21 (F8)
    0b00111000000000000000000000000000, // instruction22 (38)
    0b01111000000000000000000000000000, // instruction23 (78)
    0b11111000010000000000000000000000, // instruction24 (F8)
    0b00111000010000000000000000000000, // instruction25 (38)
    0b01111000010000000000000000000000, // instruction26 (78)
    0b11010010100000000000000000000000, // instruction27 (D2)
    0b10010001000000000000000000000000, // instruction28 (91) ADD inmediate
    0b10001011001000000000000000000000, // instruction29 (8B) ADD extended register
    0b10011011000000000111110000000000, // instruction30 (9B) MUL
    0b10110100000000000000000000000000, // instruction31 (B4) CBZ
    0b10110101000000000000000000000000, // instruction32 (B5) CBNZ
    0b10111010000000000000000000000000, // instrucyion33 (74) ADCS

};

void print_bits(uint32_t number, int bit_size);
uint32_t extract_bits(uint32_t instruction, int start, int end);
Instruction detectHLT(uint32_t instruction);
Instruction STvsLD(uint32_t instruction);
Instruction LvsR(uint32_t instruction);
Instruction bcond(uint32_t instruction);
AddWithCarryResult AddWithCarry(uint64_t x, uint64_t y, bool carry_in);
CPU_State addser(CPU_State state);
Instruction encoder(uint32_t instruction);