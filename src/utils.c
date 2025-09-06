#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void print_bits(uint32_t number, int bit_size) {
  for (int i = bit_size - 1; i >= 0; i--) {
    uint32_t bit = (number >> i) & 1;
    printf("%u", bit);
  }
  printf("\n");
}

uint32_t extract_bits(uint32_t instruction, int start, int end) {
  uint32_t mask = (1 << (end - start + 1)) - 1;
  return (instruction >> start) & mask;
}

Instruction detectHLT(uint32_t instruction) {
  uint32_t halt = extract_bits(instruction, 21, 31);
  if (halt == 0x6a2) {
    uint32_t halt = extract_bits(instruction, 0, 4);
    if (halt == 0x0) {
      return HLT;
    }
  }
  return ISNOT;
}

Instruction STvsLD(uint32_t instruction) {
  int opc = extract_bits(instruction, 22, 23);
  if (opc == 0x0) {
    return STUR;
  } else if (opc == 0x1) {
    return LDUR;
  }
  return ISNOT;
}

Instruction LvsR(uint32_t instruction) {
  uint32_t opc = extract_bits(instruction, 10, 15);
  if (opc != 0x3F) {
    return LSL;
  }
  if (opc == 0x3F) {
    return LSR;
  }
  return ISNOT;
}

Instruction bcond(uint32_t instruction) {
  int b = extract_bits(instruction, 26, 31);
  if (b == 5) {
    return B;
  }
  if (extract_bits(instruction, 24, 31) == 0x54) {
    int cond = extract_bits(instruction, 0, 3);
    if (cond == 0x0) {
      return BEQ;
    }
    if (cond == 0x1) {
      return BNE;
    }
    if (cond == 0xc) {
      return BGT;
    }
    if (cond == 0xb) {
      return BLT;
    }
    if (cond == 0xa) {
      return BGE;
    }
    if (cond == 0xd) {
      return BLE;
    }
  }
  return ISNOT;
}

Instruction SUBSvsCMP(uint32_t instruction) {
  int cond = extract_bits(instruction, 0, 4);
  if (cond == 0x1f) {
    return CMPer;
  }
  if (cond != 0x1f) {
    return SUBSer;
  }
  return ISNOT;
}

AddWithCarryResult AddWithCarry(uint64_t x, uint64_t y, bool carry_in) {
  uint64_t unsigned_sum = x + y + carry_in;
  uint64_t result = unsigned_sum & 0xFFFFFFFFFFFFFFFF;
  bool n = (result >> 63) & 1;
  bool z = (result == 0);
  AddWithCarryResult awc_result = {result, n, z};
  return awc_result;
}

CPU_State addser(CPU_State state) {
  uint32_t instruction = mem_read_32(state.PC);
  size_t m = extract_bits(instruction, 16, 20);
  size_t n = extract_bits(instruction, 5, 9);
  size_t d = extract_bits(instruction, 0, 4);
  AddWithCarryResult result = AddWithCarry(state.REGS[n], state.REGS[n], 0);
  state.REGS[d] = result.result;
  state.FLAG_N = result.flagN;
  state.FLAG_Z = result.flagZ;
  return state;
}

Instruction encoder(uint32_t instruction) {
  if (detectHLT(instruction) == HLT) {
    return HLT;
  }
  Instruction b = bcond(instruction);
  if (b != ISNOT) {
    return b;
  }
  uint32_t opcode = extract_bits(instruction, 24, 31);
  switch (opcode) {
  case 0xab:
    return ADDSer;
  case 0xb1:
    return ADDSim;
  case 0xeb:
    Instruction sb = SUBSvsCMP(instruction);
    return sb;
  case 0xf1:
    Instruction im = SUBSvsCMP(instruction);
    if (im == SUBSer) {
      return SUBSim;
    }
    if (im == CMPer) {
      return CMPim;
    }
  case 0xea:
    return ANDS;
  case 0xca:
    return EOR;
  case 0xaa:
    return ORR;
  case 0xd6:
    return BR;
  case 0xd3:
    Instruction ls = LvsR(instruction);
    if (ls == LSL) {
    }
    if (ls == LSR) {
    }
    return ls;
  case 0xf8:
    Instruction ur = STvsLD(instruction);
    if (ur == STUR) {
    }
    if (ur == LDUR) {
    }
    return ur;
  case 0x38:
    Instruction urb = STvsLD(instruction);
    if (urb == STUR) {
      return STURB;
    }
    if (urb == LDUR) {
      return LDURB;
    }
  case 0x78:
    Instruction urh = STvsLD(instruction);
    if (urh == STUR) {
      return STURH;
    }
    if (urh == LDUR) {
      return LDURH;
    }
  case 0xd2:
    return MOVZ;
  default:
    return INVALID_INSTRUCTION;
  }
  return INVALID_INSTRUCTION;
}
