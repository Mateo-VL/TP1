#include "shell.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const char *instruction_names[] = {
    "B", "BEQ", "BNE", "BGT", "BLT", "BGE", "BLE",
    "HLT", "ADDSer", "ADDSim", "SUBSer", "SUBSim", "CMPer", "CMPim",
    "ANDS", "EOR", "ORR", "BR", "LSL", "LSR", "STUR",
    "STURB", "STURH", "LDUR", "LDURB", "LDURH", "MOVZ", "ISNOT",
    "ADDim", "ADDer", "MUL", "CBZ", "CBNZ", "ADCS"};

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
    ADCS = 33,
    INVALID_INSTRUCTION = -1 // To handle invalid cases
} Instruction;

typedef struct
{
    uint64_t result;
    int flagN;
    int flagZ;
    int flagV;
    int flagC;
} AddWithCarryResult;

uint32_t extract_bits(uint32_t instruction, int start, int end)
{
    uint32_t mask = (1 << (end - start + 1)) - 1;
    return (instruction >> start) & mask;
}

Instruction detectHLT(uint32_t instruction)
{
    uint32_t halt = extract_bits(instruction, 21, 31);
    if (halt == 0x6a2)
    {
        uint32_t halt = extract_bits(instruction, 0, 4);
        if (halt == 0x0)
        {
            return HLT;
        }
    }
    return ISNOT;
}

Instruction STvsLD(uint32_t instruction)
{
    int opc = extract_bits(instruction, 22, 23);
    if (opc == 0x0)
    {
        return STUR;
    }
    else if (opc == 0x1)
    {
        return LDUR;
    }
    return ISNOT;
}

Instruction LvsR(uint32_t instruction)
{
    uint32_t opc = extract_bits(instruction, 10, 15);
    if (opc != 0x3F)
    {
        return LSL;
    }
    if (opc == 0x3F)
    {
        return LSR;
    }
    return ISNOT;
}

Instruction bcond(uint32_t instruction)
{
    int b = extract_bits(instruction, 26, 31);
    if (b == 5)
    {
        return B;
    }
    if (extract_bits(instruction, 24, 31) == 0x54)
    {
        int cond = extract_bits(instruction, 0, 3);
        if (cond == 0x0)
        {
            return BEQ;
        }
        if (cond == 0x1)
        {
            return BNE;
        }
        if (cond == 0xc)
        {
            return BGT;
        }
        if (cond == 0xb)
        {
            return BLT;
        }
        if (cond == 0xa)
        {
            return BGE;
        }
        if (cond == 0xd)
        {
            return BLE;
        }
    }
    return ISNOT;
}

Instruction SUBSvsCMP(uint32_t instruction)
{
    int cond = extract_bits(instruction, 0, 4);
    if (cond == 0x1f)
    {
        return CMPer;
    }
    if (cond != 0x1f)
    {
        return SUBSer;
    }
    return ISNOT;
}

AddWithCarryResult AddWithCarry(uint64_t x, uint64_t y, bool carry_in)
{
    AddWithCarryResult result;

    // Suma sin signo
    uint64_t unsigned_sum = x + y + carry_in;

    // Suma con signo
    int64_t signed_x = (int64_t)x;
    int64_t signed_y = (int64_t)y;
    int64_t signed_sum = signed_x + signed_y + carry_in;

    result.result = unsigned_sum;
    result.flagN = (result.result >> 63) & 1;
    result.flagZ = (result.result == 0);

    result.flagC = (unsigned_sum < x) || (unsigned_sum < y) ||
                   (carry_in && unsigned_sum == UINT64_MAX);

    bool both_positive = (signed_x >= 0) && (signed_y >= 0);
    bool both_negative = (signed_x < 0) && (signed_y < 0);
    bool result_positive = (signed_sum >= 0);

    result.flagV = (both_positive && !result_positive) ||
                   (both_negative && result_positive);

    return result;
}

int64_t SignExtend(int64_t value, int original_bits)
{
    int shift = 64 - original_bits;
    return (int64_t)(value << shift) >> shift;
}

bool ConditionHolds()
{
    uint8_t cond = extract_bits(mem_read_32(NEXT_STATE.PC), 0, 3);
    bool result = false;
    switch ((cond >> 1) & 0x7)
    {
    case 0b000:
        result = (NEXT_STATE.FLAG_Z == true);
        break;
    case 0b101:
        result = (NEXT_STATE.FLAG_N == 0);
        break;
    case 0b110:
        result = (NEXT_STATE.FLAG_N == 0 && NEXT_STATE.FLAG_Z == false);
        break;
    }
    if ((cond & 0x1) == 1 && (cond != 0b1111))
    {
        result = !result;
    }
    return result;
}

CPU_State addser()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], NEXT_STATE.REGS[m], 0);
    NEXT_STATE.REGS[d] = results.result;
    NEXT_STATE.FLAG_N = results.flagN;
    NEXT_STATE.FLAG_Z = results.flagZ;
    NEXT_STATE.FLAG_V = results.flagV;
    NEXT_STATE.FLAG_C = results.flagC;
}

CPU_State addsim()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    uint64_t imm12 = extract_bits(instruction, 10, 21);
    uint8_t shift = extract_bits(instruction, 22, 23);
    if (shift == 0b01)
    {
        imm12 = (uint64_t)imm12 << 12;
    }
    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], imm12, 0);
    NEXT_STATE.REGS[d] = results.result;
    NEXT_STATE.FLAG_N = results.flagN;
    NEXT_STATE.FLAG_Z = results.flagZ;
    NEXT_STATE.FLAG_V = results.flagV;
    NEXT_STATE.FLAG_C = results.flagC;
}

CPU_State subser()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    size_t shift = extract_bits(instruction, 10, 12);

    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], ~NEXT_STATE.REGS[m], 1);
    NEXT_STATE.REGS[d] = results.result;
    NEXT_STATE.FLAG_N = results.flagN;
    NEXT_STATE.FLAG_Z = results.flagZ;
    NEXT_STATE.FLAG_V = results.flagV;
    NEXT_STATE.FLAG_C = results.flagC;
}

CPU_State subsim()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    uint64_t imm12 = extract_bits(instruction, 10, 21);
    uint8_t shift = extract_bits(instruction, 22, 23);
    if (shift == 0b01)
    {
        imm12 = (uint64_t)imm12 << 12;
    }
    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], ~imm12, 1);
    NEXT_STATE.REGS[d] = results.result;
    NEXT_STATE.FLAG_N = results.flagN;
    NEXT_STATE.FLAG_Z = results.flagZ;
    NEXT_STATE.FLAG_V = results.flagV;
    NEXT_STATE.FLAG_C = results.flagC;
}

CPU_State cmper()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    size_t shift = extract_bits(instruction, 10, 12);

    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], ~NEXT_STATE.REGS[m], 1);
    NEXT_STATE.FLAG_N = results.flagN;
    NEXT_STATE.FLAG_Z = results.flagZ;
}

CPU_State cmpim()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    uint64_t imm12 = extract_bits(instruction, 10, 21);
    uint8_t shift = extract_bits(instruction, 22, 23);
    if (shift == 0b01)
    {
        imm12 = (uint64_t)imm12 << 12;
    }
    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], ~imm12, 1);
    NEXT_STATE.FLAG_N = results.flagN;
    NEXT_STATE.FLAG_Z = results.flagZ;
}

CPU_State ands()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    uint64_t result = NEXT_STATE.REGS[n] & NEXT_STATE.REGS[m];
    bool neg = (result >> 63) & 1;
    bool z = (result == 0);
    NEXT_STATE.REGS[d] = result;
    NEXT_STATE.FLAG_N = neg;
    NEXT_STATE.FLAG_Z = z;
}

CPU_State eor()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    NEXT_STATE.REGS[d] = NEXT_STATE.REGS[n] ^ NEXT_STATE.REGS[m];
}

CPU_State orr()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    NEXT_STATE.REGS[d] = NEXT_STATE.REGS[n] | NEXT_STATE.REGS[m];
}

CPU_State b()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    int64_t imm26 = extract_bits(instruction, 0, 25);
    int32_t value = imm26 << 2;
    int64_t offset = SignExtend(value, (int)28);
    NEXT_STATE.PC = NEXT_STATE.PC + offset;
}

CPU_State br()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t n = extract_bits(instruction, 5, 9);
    NEXT_STATE.PC = NEXT_STATE.REGS[n];
}

CPU_State bconditional()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    if (ConditionHolds(NEXT_STATE))
    {
        int64_t imm19 = extract_bits(instruction, 5, 23);
        int32_t value = imm19 << 2;
        int64_t offset = SignExtend(value, 21);
        NEXT_STATE.PC = NEXT_STATE.PC + offset;
    }
    else
    {
        NEXT_STATE.PC += 4;
    }
}

CPU_State lsl()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    uint32_t n = extract_bits(instruction, 5, 9);
    uint32_t d = extract_bits(instruction, 0, 4);
    uint32_t R = extract_bits(instruction, 16, 21);
    size_t shift = 64 - R;
    NEXT_STATE.REGS[d] = NEXT_STATE.REGS[n] << shift;
}

CPU_State lsr()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    size_t R = extract_bits(instruction, 16, 21);
    size_t shift = R;
    NEXT_STATE.REGS[d] = NEXT_STATE.REGS[n] >> shift;
}

CPU_State movz()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    uint64_t d = extract_bits(instruction, 0, 4);
    uint64_t imm16 = extract_bits(instruction, 5, 20);
    NEXT_STATE.REGS[d] = imm16;
}

CPU_State stur()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    int64_t offset = extract_bits(instruction, 12, 20);
    uint64_t n = extract_bits(instruction, 5, 9);
    uint64_t t = extract_bits(instruction, 0, 4);
    uint64_t address = NEXT_STATE.REGS[n];
    address += offset;
    uint64_t data = NEXT_STATE.REGS[t];
    uint32_t data1 = data;
    uint32_t data2 = data >> 32;
    mem_write_32(address, data1);
    mem_write_32(address + 4, data2);
    mem_write_32(address, data);
}

CPU_State sturb()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    int64_t offset = extract_bits(instruction, 12, 20);
    uint64_t n = extract_bits(instruction, 5, 9);
    uint64_t t = extract_bits(instruction, 0, 4);
    uint64_t address = NEXT_STATE.REGS[n];
    address += offset;
    uint32_t data = extract_bits(NEXT_STATE.REGS[t], 0, 7);
    mem_write_32(address, data);
}

CPU_State sturh()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    int64_t offset = extract_bits(instruction, 12, 20);
    uint64_t n = extract_bits(instruction, 5, 9);
    uint64_t t = extract_bits(instruction, 0, 4);
    uint64_t address = NEXT_STATE.REGS[n];
    address += offset;
    uint32_t data = extract_bits(NEXT_STATE.REGS[t], 0, 16);
    mem_write_32(address, data);
}

CPU_State ldur()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    int64_t offset = extract_bits(instruction, 12, 20);
    uint64_t n = extract_bits(instruction, 5, 9);
    uint64_t t = extract_bits(instruction, 0, 4);
    uint64_t address = NEXT_STATE.REGS[n];
    address += offset;
    uint32_t data1 = mem_read_32(address);
    uint32_t data2 = mem_read_32(address + 4);
    uint64_t data = data2;
    data = (data << 32) | data1;
    NEXT_STATE.REGS[t] = data;
}

CPU_State ldurb()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    int64_t offset = extract_bits(instruction, 12, 20);
    uint64_t n = extract_bits(instruction, 5, 9);
    uint64_t t = extract_bits(instruction, 0, 4);
    uint64_t address = NEXT_STATE.REGS[n];
    address += offset;
    uint32_t data = extract_bits(mem_read_32(address), 0, 7);
    NEXT_STATE.REGS[t] = data;
}

CPU_State ldurh()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    int64_t offset = extract_bits(instruction, 12, 20);
    uint64_t n = extract_bits(instruction, 5, 9);
    uint64_t t = extract_bits(instruction, 0, 4);
    uint64_t address = NEXT_STATE.REGS[n];
    address += offset;
    uint32_t data = extract_bits(mem_read_32(address), 0, 16);
    NEXT_STATE.REGS[t] = data;
}

void addim()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    uint64_t imm12 = extract_bits(instruction, 10, 21);
    uint8_t shift = extract_bits(instruction, 22, 23);
    if (shift == 0b01)
    {
        imm12 = (uint64_t)imm12 << 12;
    }
    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], imm12, 0);
    NEXT_STATE.REGS[d] = results.result;
}

void addreg()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], NEXT_STATE.REGS[m], 0);
    NEXT_STATE.REGS[d] = results.result;
}

void mul()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t d = extract_bits(instruction, 0, 4);
    size_t n = extract_bits(instruction, 5, 9);
    size_t m = extract_bits(instruction, 16, 20);
    size_t a = 31;
    NEXT_STATE.REGS[d] = NEXT_STATE.REGS[31] + (NEXT_STATE.REGS[n] * NEXT_STATE.REGS[m]);
}

void cbz()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t t = extract_bits(instruction, 0, 4);
    int64_t imm19 = extract_bits(instruction, 5, 23);
    int32_t value = imm19 << 2;
    int64_t offset = SignExtend(value, 21);
    if (NEXT_STATE.REGS[t] == 0)
    {
        NEXT_STATE.PC = NEXT_STATE.PC + offset;
    }
    else
    {
        NEXT_STATE.PC += 4;
    }
}

void cbnz()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t t = extract_bits(instruction, 0, 4);
    int64_t imm19 = extract_bits(instruction, 5, 23);
    int32_t value = imm19 << 2;
    int64_t offset = SignExtend(value, 21);
    if (NEXT_STATE.REGS[t] != 0)
    {
        NEXT_STATE.PC = NEXT_STATE.PC + offset;
    }
    else
    {
        NEXT_STATE.PC += 4;
    }
}

void adcs()
{
    uint32_t instruction = mem_read_32(NEXT_STATE.PC);
    size_t m = extract_bits(instruction, 16, 20);
    size_t n = extract_bits(instruction, 5, 9);
    size_t d = extract_bits(instruction, 0, 4);
    AddWithCarryResult results = AddWithCarry(NEXT_STATE.REGS[n], NEXT_STATE.REGS[m], NEXT_STATE.FLAG_C);
    NEXT_STATE.REGS[d] = results.result;
    NEXT_STATE.FLAG_N = results.flagN;
    NEXT_STATE.FLAG_Z = results.flagZ;
    NEXT_STATE.FLAG_V = results.flagV;
    NEXT_STATE.FLAG_C = results.flagC;
}

Instruction decode(uint32_t instruction)
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
    printf("%0x \n", opcode);
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
    case 0xba:
        return ADCS;
    default:
        return INVALID_INSTRUCTION;
    }
    return INVALID_INSTRUCTION;
}

void process_instruction()
{
    Instruction inst = decode(mem_read_32(NEXT_STATE.PC));
    printf("%i ", inst);
    printf("%s \n", instruction_names[inst]);
    switch (inst)
    {
    case HLT:
        RUN_BIT = 0;
        break;
    case ADDSer:
        addser();
        break;
    case ADDSim:
        addsim();
        break;
    case SUBSer:
        subser();
        break;
    case SUBSim:
        subsim();
        break;
    case CMPer:
        cmper();
        break;
    case CMPim:
        cmpim();
        break;
    case ANDS:
        ands();
        break;
    case EOR:
        eor();
        break;
    case ORR:
        orr();
        break;
    case B:
        b();
        break;
    case BR:
        br();
        break;
    case BEQ:
        bconditional();
        break;
    case BNE:
        bconditional();
        break;
    case BGT:
        bconditional();
        break;
    case BGE:
        bconditional();
        break;
    case BLE:
        bconditional();
        break;
    case BLT:
        bconditional();
        break;
    case LSL:
        lsl();
        break;
    case LSR:
        lsr();
        break;
    case MOVZ:
        movz();
        break;
    case STUR:
        stur();
        break;
    case STURB:
        sturb();
        break;
    case STURH:
        sturh();
        break;
    case LDUR:
        ldur();
        break;
    case LDURB:
        ldurb();
        break;
    case LDURH:
        ldurh();
        break;
    case ADDim:
        addim();
        break;
    case ADDer:
        addreg();
        break;
    case MUL:
        mul();
        break;
    case CBZ:
        cbz();
        break;
    case CBNZ:
        cbnz();
        break;
    case ADCS:
        adcs();
        break;
    default:
        break;
    }
    if (inst != B && inst != BR && inst != CBZ && inst != CBNZ &&
        !(inst >= BEQ && inst <= BLE))
    {
        NEXT_STATE.PC += 4;
    }
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory.
     *
     * Sugerencia: hagan una funcion para decode()
     *             y otra para execute()
     *
     * */
}