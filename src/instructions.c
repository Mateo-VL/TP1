#include "shell.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

CPU_State addser(CPU_State state)
{
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

Instruction encoder(uint32_t instruction)
{
  if (detectHLT(instruction) == HLT)
  {
    return HLT;
  }
  Instruction b = bcond(instruction);
  if (b != ISNOT)
  {
    return b;
  }
  uint32_t opcode = extract_bits(instruction, 24, 31);
  switch (opcode)
  {
  case 0xab:
    return ADDSer;
  case 0xb1:
    return ADDSim;
  case 0xeb:
    Instruction sb = SUBSvsCMP(instruction);
    return sb;
  case 0xf1:
    Instruction im = SUBSvsCMP(instruction);
    if (im == SUBSer)
    {
      return SUBSim;
    }
    if (im == CMPer)
    {
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
    if (ls == LSL)
    {
    }
    if (ls == LSR)
    {
    }
    return ls;
  case 0xf8:
    Instruction ur = STvsLD(instruction);
    if (ur == STUR)
    {
    }
    if (ur == LDUR)
    {
    }
    return ur;
  case 0x38:
    Instruction urb = STvsLD(instruction);
    if (urb == STUR)
    {
      return STURB;
    }
    if (urb == LDUR)
    {
      return LDURB;
    }
  case 0x78:
    Instruction urh = STvsLD(instruction);
    if (urh == STUR)
    {
      return STURH;
    }
    if (urh == LDUR)
    {
      return LDURH;
    }
  case 0xd2:
    return MOVZ;
  case 0x91:
    return ADDim;
  case 0x8b:
    return ADDer;
  case 0x9b:
    return MUL;
  case 0xb4:
    return CBZ;
  case 0xb5:
    return CBNZ;
  default:
    return INVALID_INSTRUCTION;
  }
  return INVALID_INSTRUCTION;
}
