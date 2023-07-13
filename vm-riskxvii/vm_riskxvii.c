#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define MAX_FILE_SIZE 2048;

typedef struct
{
    uint32_t registers[32];
    int32_t instructionMemory[256];
    unsigned char dataMemory[1024];
    unsigned char virtualRoutine[256];
    unsigned int pc;
    uint64_t heap[128];
    unsigned int heapsAllocated;
} VirtualMachine;
void initMemory(VirtualMachine *vm)
{
    // initialize the memory arrays
    memset(vm, 0, sizeof(VirtualMachine));
    vm->pc = 0;
    for (int i = 0; i < 256; i++)
    {
        vm->instructionMemory[i] = 0;
    }
    for (int i = 0; i < 1024; i++)
    {
        vm->dataMemory[i] = 0;
    }
    for (int i = 0; i < 256; i++)
    {
        vm->virtualRoutine[i] = 0;
    }
    for (int i = 0; i < 32; i++)
    {
        vm->registers[i] = 0;
    }
    vm->heapsAllocated = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    uint32_t opcode : 7;
    uint32_t rd : 5;
    uint32_t func3 : 3;
    uint32_t rs1 : 5;
    uint32_t rs2 : 5;
    uint32_t func7 : 7;
} RType;
typedef struct
{
    uint32_t opcode : 7;
    uint32_t rd : 5;
    uint32_t func3 : 3;
    uint32_t rs1 : 5;
    int32_t imm : 12;
} IType;
typedef struct
{
    uint32_t opcode : 7;
    int32_t imm : 12;
    uint32_t func3 : 3;
    uint32_t rs1 : 5;
    uint32_t rs2 : 5;
} SType;
typedef struct
{
    uint32_t opcode : 7;
    int32_t imm : 12;
    uint32_t func3 : 3;
    uint32_t rs1 : 5;
    uint32_t rs2 : 5;
} SBType;
typedef struct
{
    uint32_t opcode : 7;
    uint32_t rd : 5;
    int32_t imm : 20;
} UType;
typedef struct
{
    uint32_t opcode : 7;
    uint32_t rd : 5;
    int32_t imm : 20;
} UJType;

// Virtual Routine check
void regDump(VirtualMachine *vm)
{
    printf("PC = 0x%08x;\n", vm->pc);
    for (int i = 0; i < 32; i++)
    {
        printf("R[%d] = 0x%08x;\n", i, vm->registers[i]);
    }
}
void checkVRStore(VirtualMachine *vm, int addr, SType *inst)
{
    if (addr == 0x0000)
    {
        printf("%c", vm->virtualRoutine[0]);
        vm->virtualRoutine[0] = 0;
    }
    if (addr == 0x0004)
    {
        int32_t combined = 0;
        for (int i = 0; i < 4; i++)
        {
            combined = combined | (vm->virtualRoutine[4 + i] << (i * 8));
            vm->virtualRoutine[4 + i] = 0;
        }
        printf("%d", combined);
    }
    if (addr == 0x0008)
    {
        uint32_t val = vm->registers[inst->rs2];
        for (int i = 0; i < 4; i++)
        {
            vm->virtualRoutine[8 + i] = (val >> (i * 8)) & 0xFF;
        }

        uint32_t combined = 0;
        for (int i = 0; i < 4; i++)
        {
            combined = combined | (vm->virtualRoutine[8 + i] << (i * 8));
            vm->virtualRoutine[8 + i] = 0;
        }
        printf("%x", combined);
    }
    if (addr == 0x000C)
    {
        vm->virtualRoutine[3] = 0;
        printf("CPU Halt Requested\n");
        exit(0);
    }
    if (addr == 0x0012)
    {
        char input;
        scanf("%c", &input);
    }
    if (addr == 0x0020)
    {
        printf("%d", vm->pc);
    }
    if (addr == 0x0024)
    {
        regDump(vm);
    }
    if (addr == 0x0028)
    {
        uint32_t v = (uint32_t)vm->virtualRoutine[40];
        printf("%x", vm->dataMemory[v]);
    }
    if (addr == 0x0030)
    {
        // get the value being stored
        uint32_t val = vm->virtualRoutine[48];
        if (vm->heapsAllocated + ((val + (64 - 1)) / 64) <= 128)
        {

            if (vm->heapsAllocated == 0)
            {
                vm->registers[28] = 46848;
            }
            else
            {
                vm->registers[28] += ((val + (64 - 1)) / 64) * 64;
            }
            vm->heapsAllocated += (val + (64 - 1)) / 64;
        }
        else{
            vm->registers[28] = 0;
        }
    }
    if(addr == 0x0034){
        uint32_t val = vm->virtualRoutine[52];
        vm->heapsAllocated -= (val + (64 - 1)) / 64;
    }
}
void loadVR(VirtualMachine *vm, int addr, IType *inst)
{
    if (addr == 0x0812)
    {
        char c;
        scanf("%c", &c);
        vm->registers[inst->rd] = c;
        vm->pc += 4;
    }
    else
    {
        int32_t value;
        scanf("%d", &value);
        vm->registers[inst->rd] = value;
        vm->pc += 4;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RType binaryToRType(int32_t hex)
{
    RType instruction;
    instruction.opcode = (hex >> 0) & 0x7F;
    instruction.rd = (hex >> 7) & 0x1F;
    instruction.func3 = (hex >> 12) & 0x7;
    instruction.rs1 = (hex >> 15) & 0x1F;
    instruction.rs2 = (hex >> 20) & 0x1F;
    instruction.func7 = (hex >> 25) & 0x7F;
    return instruction;
}
IType binaryToIType(int32_t hex)
{
    IType instruction;
    instruction.opcode = (hex >> 0) & 0x7F;
    instruction.rd = (hex >> 7) & 0x1F;
    instruction.func3 = (hex >> 12) & 0x7;
    instruction.rs1 = (hex >> 15) & 0x1F;
    instruction.imm = (hex >> 20) & 0xFFF;
    if ((instruction.imm >> 11) & 1)
    {
        instruction.imm |= 0xFFFFF000;
    }
    return instruction;
}

SType binaryToSType(int32_t hex)
{
    SType instruction;
    instruction.opcode = hex & 0x7F;
    instruction.imm = ((hex >> 20) & 0xFE0) | ((hex >> 7) & 0x1F);
    if ((instruction.imm >> 11) & 1)
    {
        instruction.imm |= 0xFFFFF000;
    }
    instruction.func3 = (hex >> 12) & 0x7;
    instruction.rs1 = (hex >> 15) & 0x1F;
    instruction.rs2 = (hex >> 20) & 0x1F;
    return instruction;
}
SBType binaryToSBType(int32_t hex)
{
    SBType instruction;
    instruction.opcode = hex & 0x7F;
    instruction.imm = (((hex >> 31) & 0x1) << 11) |
                      (((hex >> 7) & 0x1) << 10) |
                      (((hex >> 8) & 0xF) << 0) |
                      (((hex >> 25) & 0x3F) << 4);
    if ((instruction.imm >> 11) & 1)
    {
        instruction.imm |= 0xFFFFF000;
    }
    instruction.func3 = (hex >> 12) & 0x7;
    instruction.rs1 = (hex >> 15) & 0x1F;
    instruction.rs2 = (hex >> 20) & 0x1F;
    return instruction;
}
UType binaryToUType(int32_t hex)
{
    UType instruction;
    instruction.opcode = hex & 0x7F;
    instruction.rd = (hex >> 7) & 0x1F;
    instruction.imm = (hex >> 12) & 0xFFFFF;
    if (instruction.imm >> 19 & 1)
    {
        instruction.imm |= 0xFFF00000;
    }
    return instruction;
}
UJType binaryToUJType(int32_t hex)
{
    UJType instruction;
    instruction.opcode = hex & 0x7F;
    instruction.rd = (hex >> 7) & 0x1F;
    instruction.imm = (((hex >> 31) & 0x1) << 19) |
                      (((hex >> 12) & 0xFF) << 11) |
                      (((hex >> 20) & 0x1) << 10) |
                      (((hex >> 21) & 0x3FF) << 0);
    if (instruction.imm >> 19 & 1)
    {
        instruction.imm |= 0xFFF00000;
    }
    return instruction;
}
int32_t checkOpCode(int32_t instruction)
{
    int32_t opcode = instruction & 0x7F;
    return opcode;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// R-type instructions
void add(RType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] + vm->registers[inst->rs2];
    vm->pc += 4;
}
void sub(RType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] - vm->registers[inst->rs2];
    vm->pc += 4;
}
void xor (RType * inst, VirtualMachine *vm) {
    vm->registers[inst->rd] = vm->registers[inst->rs1] ^ vm->registers[inst->rs2];
    vm->pc += 4;
} void or
    (RType * inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] | vm->registers[inst->rs2];
    vm->pc += 4;
}
void and (RType * inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] & vm->registers[inst->rs2];
    vm->pc += 4;
}
void sll(RType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] << vm->registers[inst->rs2];
    vm->pc += 4;
}
void srl(RType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] >> vm->registers[inst->rs2];
    vm->pc += 4;
}
void sra(RType *inst, VirtualMachine *vm)
{
    int32_t shift = vm->registers[inst->rs2];
    int32_t save = vm->registers[inst->rs1] << (32 - shift);
    save = save >> (32 - shift);
    vm->registers[inst->rd] = vm->registers[inst->rs1] >> shift;
    vm->registers[inst->rd] = vm->registers[inst->rd] | save;
    vm->pc += 4;
}
void slt(RType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = (vm->registers[inst->rs1] < vm->registers[inst->rs2]) ? 1 : 0;
    vm->pc += 4;
}
void sltu(RType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = ((uint32_t)vm->registers[inst->rs1] < (uint32_t)vm->registers[inst->rs2]) ? 1 : 0;
    vm->pc += 4;
}
// I-type instructions
void addi(IType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] + inst->imm;
    vm->pc += 4;
}
void xori(IType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] ^ inst->imm;
    vm->pc += 4;
}
void ori(IType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] | inst->imm;
    vm->pc += 4;
}
void andi(IType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->registers[inst->rs1] & inst->imm;
    vm->pc += 4;
}
void lb(IType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047)
    {
        loadVR(vm, addr, inst);
    }
    else
    {
        addr = addr - 1024;
        int8_t val = vm->dataMemory[addr];
        int32_t sign_ext_val = (val & 0x80) ? (val | 0xFFFFFF00) : val;
        vm->registers[inst->rd] = sign_ext_val;
        vm->pc += 4;
    }
}

void lh(IType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047)
    {
        loadVR(vm, addr, inst);
    }
    else
    {
        addr = addr - 1024;
        int16_t val = 0;
        val = vm->dataMemory[addr] | (vm->dataMemory[addr + 1] << 8);
        int32_t sign_ext_val = (val & 0x8000) ? (val | 0xFFFF0000) : val;

        vm->registers[inst->rd] = sign_ext_val;
        vm->pc += 4;
    }
}

void lw(IType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047)
    {
        loadVR(vm, addr, inst);
    }
    else
    {
        addr = addr - 1024;
        int32_t val = vm->dataMemory[addr] | (vm->dataMemory[addr + 1] << 8) |
                      (vm->dataMemory[addr + 2] << 16) | (vm->dataMemory[addr + 3] << 24);
        vm->registers[inst->rd] = val;
        vm->pc += 4;
    }
}
void lbu(IType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047)
    {
        loadVR(vm, addr, inst);
    }
    else
    {
        addr = addr - 1024;
        vm->registers[inst->rd] = (uint8_t)vm->dataMemory[addr];
        vm->pc += 4;
    }
}
void lhu(IType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047)
    {
        loadVR(vm, addr, inst);
    }
    else
    {
        addr = addr - 1024;
        uint16_t val = vm->dataMemory[addr] | (vm->dataMemory[addr + 1] << 8);
        vm->registers[inst->rd] = (uint16_t)val;
        vm->pc += 4;
    }
}

void slti(IType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = (vm->registers[inst->rs1] < inst->imm) ? 1 : 0;
    vm->pc += 4;
}
void sltiu(IType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = ((uint32_t)vm->registers[inst->rs1] < (uint32_t)inst->imm) ? 1 : 0;
    vm->pc += 4;
}
void jalr(IType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->pc + 4;
    vm->pc = vm->registers[inst->rs1] + inst->imm;
}
// S-type instructions
void sb(SType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047)
    {
        int8_t val = vm->registers[inst->rs2];
        addr = (addr - 2048);
        vm->virtualRoutine[addr / 4] = val;
        checkVRStore(vm, addr, inst);
    }
    else
    {
        int8_t val = vm->registers[inst->rs2];
        addr = addr - 1024;
        vm->dataMemory[addr] = val;
    }
    vm->pc += 4;
}
void sh(SType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047 && addr < 46848)
    {
        int16_t val = vm->registers[inst->rs2];
        int8_t val_low = (val >> 0) & 0xFF;
        int8_t val_high = (val >> 8) & 0xFF;
        addr = (addr - 2048);
        vm->virtualRoutine[addr] = val_low;
        vm->virtualRoutine[addr + 1] = val_high;
        checkVRStore(vm, addr, inst);
    }
    else
    {
        if (addr > 46847 && addr < 46848)
        {
            addr = addr - 46848;
            int32_t val = vm->registers[inst->rs2];
            vm->heap[addr] = val;
        }
        else
        {
            if (addr > 46847)
            {
                addr = addr - 46848;
                int32_t val = vm->registers[inst->rs2];
                vm->heap[addr] = val;
            }
            else
            {
                int16_t val = vm->registers[inst->rs2];
                int8_t val_low = (val >> 0) & 0xFF;
                int8_t val_high = (val >> 8) & 0xFF;
                addr = addr - 1024;
                vm->dataMemory[addr] = val_low;
                vm->dataMemory[addr + 1] = val_high;
            }
        }
    }
    vm->pc += 4;
}
void sw(SType *inst, VirtualMachine *vm)
{
    int32_t addr = vm->registers[inst->rs1] + inst->imm;
    if (addr > 2047 && addr < 46848)
    {
        addr = (addr - 2048);
        int32_t val = vm->registers[inst->rs2];
        int8_t val_low = (val >> 0) & 0xFF;
        int8_t val_high = (val >> 8) & 0xFF;
        int8_t val_higher = (val >> 16) & 0xFF;
        int8_t val_highest = (val >> 24) & 0xFF;
        vm->virtualRoutine[addr] = val_low;
        vm->virtualRoutine[addr + 1] = val_high;
        vm->virtualRoutine[addr + 2] = val_higher;
        vm->virtualRoutine[addr + 3] = val_highest;
        checkVRStore(vm, addr, inst);
    }
    else
    {
        if (addr > 46847)
        {
            addr = addr - 46848;
            int32_t val = vm->registers[inst->rs2];
            vm->heap[addr] = val;
        }
        else
        {
            addr = addr - 1024;
            int32_t val = vm->registers[inst->rs2];
            int8_t val_low = (val >> 0) & 0xFF;
            int8_t val_high = (val >> 8) & 0xFF;
            int8_t val_higher = (val >> 16) & 0xFF;
            int8_t val_highest = (val >> 24) & 0xFF;
            vm->dataMemory[addr] = val_low;
            vm->dataMemory[addr + 1] = val_high;
            vm->dataMemory[addr + 2] = val_higher;
            vm->dataMemory[addr + 3] = val_highest;
        }
    }
    vm->pc += 4;
}
// SB-type instructions
void beq(SBType *inst, VirtualMachine *vm)
{
    if (vm->registers[inst->rs1] == vm->registers[inst->rs2])
    {
        vm->pc += inst->imm << 1;
    }
    else
    {
        vm->pc += 4;
    }
}
void bne(SBType *inst, VirtualMachine *vm)
{
    if (vm->registers[inst->rs1] != vm->registers[inst->rs2])
    {
        vm->pc += inst->imm << 1;
    }
    else
    {
        vm->pc += 4;
    }
}
void blt(SBType *inst, VirtualMachine *vm)
{
    if (vm->registers[inst->rs1] < vm->registers[inst->rs2])
    {
        vm->pc += inst->imm << 1;
    }
    else
    {
        vm->pc += 4;
    }
}
void bltu(SBType *inst, VirtualMachine *vm)
{
    if ((uint32_t)vm->registers[inst->rs1] < (uint32_t)vm->registers[inst->rs2])
    {
        vm->pc += inst->imm << 1;
    }
    else
    {
        vm->pc += 4;
    }
}
void bge(SBType *inst, VirtualMachine *vm)
{
    if (vm->registers[inst->rs1] >= vm->registers[inst->rs2])
    {
        vm->pc += inst->imm << 1;
    }
    else
    {
        vm->pc += 4;
    }
}
void bgeu(SBType *inst, VirtualMachine *vm)
{
    if ((uint32_t)vm->registers[inst->rs1] >= (uint32_t)vm->registers[inst->rs2])
    {
        vm->pc += inst->imm << 1;
    }
    else
    {
        vm->pc += 4;
    }
}
// U-type instructions
void lui(UType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = inst->imm << 12;
    vm->pc += 4;
}
// UJ-type instructions
void jal(UJType *inst, VirtualMachine *vm)
{
    vm->registers[inst->rd] = vm->pc + 4;
    vm->pc += inst->imm << 1;
}
//////////////////////////////////////////////////////////////////////
void executeRType(RType *inst, VirtualMachine *vm)
{
    if (inst->func3 == 0b000 && inst->func7 == 0b0000000)
    {
        add(inst, vm);
    }
    else if (inst->func3 == 0b101 && inst->func7 == 0b0100000)
    {
        sra(inst, vm);
    }
    else if (inst->func3 == 0b000 && inst->func7 == 0b0100000)
    {
        sub(inst, vm);
    }
    else if (inst->func3 == 0b001 && inst->func7 == 0b0000000)
    {
        sll(inst, vm);
    }
    else if (inst->func3 == 0b010 && inst->func7 == 0b0000000)
    {
        slt(inst, vm);
    }
    else if (inst->func3 == 0b011 && inst->func7 == 0b0000000)
    {
        sltu(inst, vm);
    }
    else if (inst->func3 == 0b100 && inst->func7 == 0b0000000)
    {
        xor(inst, vm);
    }
    else if (inst->func3 == 0b101 && inst->func7 == 0b0000000)
    {
        srl(inst, vm);
    }
    else if (inst->func3 == 0b110 && inst->func7 == 0b0000000)
    {
        or (inst, vm);
    }
    else if (inst->func3 == 0b111 && inst->func7 == 0b0000000)
    {
        and(inst, vm);
    }
    else
    {
        printf("Invalid instruction.");
    }
    return;
}
void executeIType(IType *inst, VirtualMachine *vm)
{
    if (inst->opcode == 0x13)
    {
        if (inst->func3 == 0b000)
        {
            addi(inst, vm);
        }
        else if (inst->func3 == 0b100)
        {
            xori(inst, vm);
        }
        else if (inst->func3 == 0b110)
        {
            ori(inst, vm);
        }
        else if (inst->func3 == 0b111)
        {
            andi(inst, vm);
        }
        else if (inst->func3 == 0b010)
        {
            slti(inst, vm);
        }
        else if (inst->func3 == 0b011)
        {
            sltiu(inst, vm);
        }
    }
    else if (inst->opcode == 0x3)
    {
        if (inst->func3 == 0b000)
        {
            lb(inst, vm);
        }
        else if (inst->func3 == 0b001)
        {
            lh(inst, vm);
        }
        else if (inst->func3 == 0b010)
        {
            lw(inst, vm);
        }
        else if (inst->func3 == 0b100)
        {
            lbu(inst, vm);
        }
        else if (inst->func3 == 0b101)
        {
            lhu(inst, vm);
        }
    }
    else if (inst->opcode == 0x67)
    {
        jalr(inst, vm);
    }
    return;
}
void executeSType(SType *inst, VirtualMachine *vm)
{
    if (inst->func3 == 0b000)
    {
        sb(inst, vm);
    }
    else if (inst->func3 == 0b001)
    {
        sh(inst, vm);
    }
    else if (inst->func3 == 0b010)
    {
        sw(inst, vm);
    }
    return;
}
void executeUType(UType *inst, VirtualMachine *vm)
{
    lui(inst, vm);
    return;
}
void executeSBType(SBType *inst, VirtualMachine *vm)
{
    if (inst->func3 == 0b000)
    {
        beq(inst, vm);
    }
    else if (inst->func3 == 0b001)
    {
        bne(inst, vm);
    }
    else if (inst->func3 == 0b100)
    {
        blt(inst, vm);
    }
    else if (inst->func3 == 0b101)
    {
        bge(inst, vm);
    }
    else if (inst->func3 == 0b110)
    {
        bltu(inst, vm);
    }
    else if (inst->func3 == 0b111)
    {
        bgeu(inst, vm);
    }
    return;
}
void executeUJType(UJType *inst, VirtualMachine *vm)
{
    jal(inst, vm);
    return;
}

int main(int argc, char *argv[])
{
    char *filename = argv[1];
    VirtualMachine vm;
    initMemory(&vm);
    FILE *fp = fopen(filename, "r"); // Open file in binary mode
    if (fp == NULL)
    {
        perror("Error opening file");
        exit(0);
    }
    int32_t buffer[2048];
    size_t file_size = fread(buffer, sizeof(int32_t), 512, fp);
    int temp = 0;
    for (int i = 0; i < file_size; i++)
    {
        if (i < 1024)
        {
            temp = buffer[i];
            vm.instructionMemory[i] = temp;
        }
        else
        {
            temp = buffer[i];
            vm.dataMemory[i - 1024] = temp;
        }
    }

    fclose(fp);
    int32_t hex = vm.instructionMemory[0];
    int tempPC = 0;
    // test instruction
    while (1)
    {
        vm.registers[0] = 0;
        if (tempPC == vm.pc && vm.pc != 0)
        {
            printf("Infinite loop detected at %08x, %d \n.", vm.instructionMemory[vm.pc / 4], vm.pc);
            exit(0);
        }
        tempPC = vm.pc;
        hex = vm.instructionMemory[vm.pc / 4];
        int32_t opCode = checkOpCode(hex);
        switch (opCode)
        {
        case 0x33: // 0110011
            RType rInstruction = binaryToRType(hex);
            // printf("RType instruction: %08x at pc %d\n", hex, vm.pc);
            // printf("opcode: %d funct3: %d funct7: %d rd: %d rs1: %d rs2: %d\n", rInstruction.opcode, rInstruction.func3, rInstruction.func7, rInstruction.rd, rInstruction.rs1, rInstruction.rs2);
            executeRType(&rInstruction, &vm);
            break;
        case 0x13: // 0010011
            IType iInstruction = binaryToIType(hex);
            // printf("IType instruction: %08x at pc %d\n", hex, vm.pc);
            // printf("opcode: %d funct3: %d rd: %d rs1: %d imm: %d\n", iInstruction.opcode, iInstruction.func3, iInstruction.rd, iInstruction.rs1, iInstruction.imm);
            executeIType(&iInstruction, &vm);
            break;
        case 0x3: // 0000011
            IType iInstructionOp1 = binaryToIType(hex);
            // printf("IType1 instruction: %08x at pc %d\n", hex, vm.pc);
            // printf("opcode: %d funct3: %d rd: %d rs1: %d imm: %d\n", iInstructionOp1.opcode, iInstructionOp1.func3, iInstructionOp1.rd, iInstructionOp1.rs1, iInstructionOp1.imm);
            executeIType(&iInstructionOp1, &vm);
            break;
        case 0x67: // 1100111
            IType iInstructionOp2 = binaryToIType(hex);
            // printf("IType2 instruction: %08x a:t pc %d\n", hex, vm.pc);
            // printf("opcode: %d funct3: %d rd %d rs1: %d imm: %d\n", iInstructionOp2.opcode, iInstructionOp2.func3, iInstructionOp2.rd, iInstructionOp2.rs1, iInstructionOp2.imm);
            executeIType(&iInstructionOp2, &vm);
            break;
        case 0x37: // 0110111
            UType uInstruction = binaryToUType(hex);
            // printf("UType instruction: %08x at pc %d\n", hex, vm.pc);
            // printf("opcode: %d rd: %d imm: %d\n", uInstruction.opcode, uInstruction.rd, uInstruction.imm);
            executeUType(&uInstruction, &vm);
            break;
        case 0x23: // 0100011
            SType sInstruction = binaryToSType(hex);
            // printf("SType instruction: %08x at pc %d\n", hex, vm.pc);
            // printf("opcode: %d funct3: %d rs1: %d rs2: %d imm: %d\n", sInstruction.opcode, sInstruction.func3, sInstruction.rs1, sInstruction.rs2, sInstruction.imm);
            executeSType(&sInstruction, &vm);
            break;
        case 0x63: // 1100011
            SBType sbInstruction = binaryToSBType(hex);
            // printf("SBType instruction: %08x at pc %d\n", hex, vm.pc);
            // printf("opcode: %d funct3: %d rs1: %d rs2: %d imm: %d\n", sbInstruction.opcode, sbInstruction.func3, sbInstruction.rs1, sbInstruction.rs2, sbInstruction.imm);
            executeSBType(&sbInstruction, &vm);
            break;
        case 0x6F: // 1101111
            UJType ujInstruction = binaryToUJType(hex);
            // printf("UJType instruction: %08x at pc %d\n", hex, vm.pc);
            // printf("opcode: %d rd: %d imm: %d\n", ujInstruction.opcode, ujInstruction.rd, (int32_t)ujInstruction.imm);
            executeUJType(&ujInstruction, &vm);
            break;
        default:
            printf("Instruction Not Implemented: 0x%08x\n", hex);
            regDump(&vm);
            exit(0);
        }
        vm.registers[0] = 0;
    }
    return 0;
}
// 00000000000 01010 100 01111 0000011
