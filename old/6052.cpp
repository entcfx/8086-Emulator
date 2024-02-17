#include <iostream>
#include <array>
#include <bitset>
#include <optional>
#include <vector>
#include "header.h"

class AddressBus {
public:
    Word address;

    AddressBus() : address(0) {}
    void setAddress(u32 addr) {
        address = addr;
    }
};

class DataBus {
public:
    Word data;

    DataBus() : data(0) {}
    void setData(Word d) { data = d; }
    Word getData() const { return data; }
};

class Memory {
public:
    std::vector<Byte> mem;

    Memory(size_t size) : mem(size + 1) {}

    void read(AddressBus& addressBus, DataBus& dataBus) {
        if (addressBus.address < mem.size()) {
            dataBus.setData(mem[addressBus.address]);
        }
    }

    void write(AddressBus& addressBus, DataBus& dataBus) {
        if (addressBus.address < mem.size()) {
            mem[addressBus.address] = dataBus.getData();
        }
    }
};


class CPU {
public:
    u32 cycles;
    union {
        struct { Word AX, BX, CX, DX; };
        struct { Byte AL, AH, BL, BH, CL, CH, DL, DH; };
    };

    Word SP, BP, SI, DI;

    Word IP;

    // Flags bytearray
    Byte CF : 1; // Carry flag
    Byte ZF : 1; // Zero flag
    Byte SF : 1; // Sign flag
    Byte OF : 1; // Overflow flag
    Byte DF : 1; // Decimal flag
    Byte IF : 1; // Interrupt enabled flag
    Byte RF1 : 1; // Reserved flag 1
    Byte RF2 : 1; // Reserved flag 2

    Word IDTR;

    CPU(Memory& mem) : AX(0), BX(0), CX(0), DX(0),
                    SP(0), BP(0), SI(0), DI(0),
                    IP(0), memory(mem), addressBus(), dataBus(), cycles(0), IDTR(0), { reset(); }
    
    void reset() { // During reset, cycles are ignored
        AX = BX = CX = DX = 0;
        SP = BP = SI = DI = 0;
        IP = 0;

        // Get reset vector from FFFF0H
        IP = readFromMemory(0xFFFF);
    }

    void run() {
        while (cycles > 0) {
            Byte opcode = fetchNextInstruction();
            if (opcode == -1) {
                std::cout << "Cycles exhausted, halting CPU." << std::endl;
                break;
            }
            executeInstruction(opcode);
            // Check if cycles are exhausted
            if (cycles <= 0) {
                std::cout << "Cycles exhausted, halting CPU." << std::endl;
                break;
            }
        }
    }

    void executeInstruction(Byte opcode) {
        if (cycles <= 0) return;
        switch (opcode)
        {
        case 0x00: // NOP
            cycles--;
            break;
        case 0x01: {
            // MOV reg8, reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            setRegisterByCode8(reg2, reg1Value);
            cycles -= 3;
            break;
        }
        case 0x02: {
            // MOV r16, r16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            setRegisterByCode16(reg2, reg1Value);
            cycles -= 3;
            break;
        }
        case 0x03: {
            // MOV reg16, ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            setRegisterByCode16(reg, value);
            cycles -= 6;
            break;
        }
        case 0x04: {
            // MOV ram16, reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            writeToMemory(addr, value);
            cycles -= 6;
            break;
        }
        case 0x05: {
            // MOV reg8, imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            setRegisterByCode8(reg, value);
            cycles -= 4;
            break;
        }
        case 0x06: {
            // MOV reg16, imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            setRegisterByCode16(reg, value);
            cycles -= 5;
            break;
        }
        case 0x07: {
            // ADD reg8, reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value + reg2Value;
            setRegisterByCode8(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x08: {
            // ADD reg16, reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value + reg2Value;
            setRegisterByCode16(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x09: {
            // ADD reg16, ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue + value;
            setRegisterByCode16(reg, result);
            cycles -= 6;
            break;
        }
        case 0x0A: {
            // ADD ram16, reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue + value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x0B: {
            // ADD reg8, imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue + value;
            setRegisterByCode8(reg, result);
            cycles -= 4;
            break;
        }
        case 0x0C: {
            // ADD reg16, imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue + value;
            setRegisterByCode16(reg, result);
            cycles -= 5;
            break;
        }
        case 0x0D: {
            // SUB reg8, reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value - reg2Value;
            setRegisterByCode8(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x0E: {
            // SUB reg16, reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value - reg2Value;
            setRegisterByCode16(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x0F: {
            // SUB reg16, ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue - value;
            setRegisterByCode16(reg, result);
            cycles -= 6;
            break;
        }
        case 0x10: {
            // SUB ram16, reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue - value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x11: {
            // SUB reg8, imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue - value;
            setRegisterByCode8(reg, result);
            cycles -= 4;
            break;
        }
        case 0x12: {
            // SUB reg16, imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue - value;
            setRegisterByCode16(reg, result);
            cycles -= 5;
            break;
        }
        case 0x13: {
            // MUL reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Word result = reg1Value * reg2Value;
            setRegisterByCode16(AXH, result);
            cycles -= 3;
            break;
        }
        case 0x14: {
            // MUL reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value * reg2Value;
            setRegisterByCode16(AXH, result);
            cycles -= 3;
            break;
        }
        case 0x15: {
            // MUL reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue * value;
            setRegisterByCode16(AXH, result);
            cycles -= 6;
            break;
        }
        case 0x16: {
            // MUL ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue * value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x17: {
            // MUL reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Word result = regValue * value;
            setRegisterByCode16(AXH, result);
            cycles -= 4;
            break;
        }
        case 0x18: {
            // MUL reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue * value;
            setRegisterByCode16(AXH, result);
            cycles -= 5;
            break;
        }
        case 0x19: {
            // DIV reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Word result = reg1Value / reg2Value;
            setRegisterByCode16(AXH, result);
            cycles -= 3;
            break;
        }
        case 0x1A: {
            // DIV reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value / reg2Value;
            setRegisterByCode16(AXH, result);
            cycles -= 3;
            break;
        }
        case 0x1B: {
            // DIV reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue / value;
            setRegisterByCode16(AXH, result);
            cycles -= 6;
            break;
        }
        case 0x1C: {
            // DIV ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue / value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x1D: {
            // DIV reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Word result = regValue / value;
            setRegisterByCode16(AXH, result);
            cycles -= 4;
            break;
        }
        case 0x1E: {
            // DIV reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue / value;
            setRegisterByCode16(AXH, result);
            cycles -= 5;
            break;
        }
        case 0x1F: {
            // AND reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value & reg2Value;
            setRegisterByCode8(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x20: {
            // AND reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value & reg2Value;
            setRegisterByCode16(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x21: {
            // AND reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue & value;
            setRegisterByCode16(reg, result);
            cycles -= 6;
            break;
        }
        case 0x22: {
            // AND ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue & value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x23: {
            // AND reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue & value;
            setRegisterByCode8(reg, result);
            cycles -= 4;
            break;
        }
        case 0x24: {
            // AND reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue & value;
            setRegisterByCode16(reg, result);
            cycles -= 5;
            break;
        }
        case 0x25: {
            // OR reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value | reg2Value;
            setRegisterByCode8(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x26: {
            // OR reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value | reg2Value;
            setRegisterByCode16(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x27: {
            // OR reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue | value;
            setRegisterByCode16(reg, result);
            cycles -= 6;
            break;
        }
        case 0x28: {
            // OR ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue | value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x29: {
            // OR reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue | value;
            setRegisterByCode8(reg, result);
            cycles -= 4;
            break;
        }
        case 0x2A: {
            // OR reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue | value;
            setRegisterByCode16(reg, result);
            cycles -= 5;
            break;
        }
        case 0x2B: {
            // XOR reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value ^ reg2Value;
            setRegisterByCode8(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x2C: {
            // XOR reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value ^ reg2Value;
            setRegisterByCode16(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x2D: {
            // XOR reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue ^ value;
            setRegisterByCode16(reg, result);
            cycles -= 6;
            break;
        }
        case 0x2E: {
            // XOR ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue ^ value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x2F: {
            // XOR reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue ^ value;
            setRegisterByCode8(reg, result);
            cycles -= 4;
            break;
        }
        case 0x30: {
            // XOR reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Word value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue ^ value;
            setRegisterByCode16(reg, result);
            cycles -= 5;
            break;
        }
        case 0x31: {
            // SHL reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value << reg2Value;
            setRegisterByCode8(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x32: {
            // SHL reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value << reg2Value;
            setRegisterByCode16(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x33: {
            // SHL reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue << value;
            setRegisterByCode16(reg, result);
            cycles -= 6;
            break;
        }
        case 0x34: {
            // SHL ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Byte addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue << value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x35: {
            // SHL reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue << value;
            setRegisterByCode8(reg, result);
            cycles -= 4;
            break;
        }
        case 0x36: {
            // SHL reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Byte value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue << value;
            setRegisterByCode16(reg, result);
            cycles -= 5;
            break;
        }
        case 0x37: {
            // SHR reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value >> reg2Value;
            setRegisterByCode8(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x38: {
            // SHR reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value >> reg2Value;
            setRegisterByCode16(reg1, result);
            cycles -= 3;
            break;
        }
        case 0x39: {
            // SHR reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue >> value;
            setRegisterByCode16(reg, result);
            cycles -= 6;
            break;
        }
        case 0x3A: {
            // SHR ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Byte addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue >> value;
            writeToMemory(addr, result);
            cycles -= 6;
            break;
        }
        case 0x3B: {
            // SHR reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue >> value;
            setRegisterByCode8(reg, result);
            cycles -= 4;
            break;
        }
        case 0x3C: {
            // SHR reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Byte value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue >> value;
            setRegisterByCode16(reg, result);
            cycles -= 5;
            break;
        }
        case 0x3D: {
            // CMP reg8 reg8
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Byte reg1Value = getRegisterByCode8(reg1);
            Byte reg2Value = getRegisterByCode8(reg2);
            Byte result = reg1Value - reg2Value;
            cycles -= 3;
            break;
        }
        case 0x3E: {
            // CMP reg16 reg16
            if (cycles <= 3) return;
            Byte reg1 = fetchNextInstruction();
            Byte reg2 = fetchNextInstruction();
            Word reg1Value = getRegisterByCode16(reg1);
            Word reg2Value = getRegisterByCode16(reg2);
            Word result = reg1Value - reg2Value;
            cycles -= 3;
            break;
        }
        case 0x3F: {
            // CMP reg16 ram16
            if (cycles <= 6) return;
            Byte reg = fetchNextInstruction();
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue - value;
            cycles -= 6;
            break;
        }
        case 0x40: {
            // CMP ram16 reg16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Byte reg = fetchNextInstruction();
            Byte addr = (addrHigh << 8) | addrLow;
            Word value = getRegisterByCode16(reg);
            Word memValue = readFromMemory(addr);
            Word result = memValue - value;
            cycles -= 6;
            break;
        }
        case 0x41: {
            // CMP reg8 imm8
            if (cycles <= 4) return;
            Byte reg = fetchNextInstruction();
            Byte value = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            Byte result = regValue - value;
            cycles -= 4;
            break;
        }
        case 0x42: {
            // CMP reg16 imm16
            if (cycles <= 5) return;
            Byte reg = fetchNextInstruction();
            Byte valueLow = fetchNextInstruction();
            Byte valueHigh = fetchNextInstruction();
            Byte value = (valueHigh << 8) | valueLow;
            Word regValue = getRegisterByCode16(reg);
            Word result = regValue - value;
            cycles -= 5;
            break;
        }
        case 0x43: {
            // JMP imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            IP = addr;
            cycles -= 5;
            break;
        }
        case 0x44: {
            // JNZ imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            if (ZF == 0) IP = addr;
            cycles -= 5;
            break;
        }
        case 0x45: {
            // JZ imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            if (ZF == 1) IP = addr;
            cycles -= 5;
            break;
        }
        case 0x46: {
            // JC imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            if (CF == 1) IP = addr;
            cycles -= 5;
            break;
        }
        case 0x47: {
            // JNC imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            if (CF == 0) IP = addr;
            cycles -= 5;
            break;
        }
        case 0x48: {
            // JS imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            if (SF == 1) IP = addr;
            cycles -= 5;
            break;
        }
        case 0x49: {
            // JNS imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            if (SF == 0) IP = addr;
            cycles -= 5;
            break;
        }
        case 0x4A: {
            // JD imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            if (DF == 1) {
                Word addr = (addrHigh << 8) | addrLow;
                IP = addr;
            }
            cycles -= 5;
            break;
        }
        case 0x4B: {
            // JND imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            if (DF == 0) {
                Word addr = (addrHigh << 8) | addrLow;
                IP = addr;
            }
            cycles -= 5;
            break;
        }
        case 0x4C: {
            // JO imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            if (OF == 1) {
                Word addr = (addrHigh << 8) | addrLow;
                IP = addr;
            }
            cycles -= 5;
            break;
        }
        case 0x4D: {
            // JNO imm16
            if (cycles <= 5) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            if (OF == 0) {
                Word addr = (addrHigh << 8) | addrLow;
                IP = addr;
            }
            cycles -= 5;
            break;
        }
        case 0x4E: {
            // CALL imm16
            if (cycles <= 7) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            // Push IP to the stack
            SP -= 2;
            writeToMemory(SP, IP);
            IP = addr;
            cycles -= 7;
            break;
        }
        case 0x4F: {
            // RET
            if (cycles <= 6) return;
            // Pop IP from the stack
            IP = readFromMemory(SP);
            SP += 2;
            cycles -= 6;
            break;
        }
        case 0x50: {
            // HLT
            if (cycles <= 1) return;
            cycles = 0;
            break;
        }
        case 0x51: {
            // PUSH reg8
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            SP -= 1;
            writeToMemory(SP, regValue);
            cycles -= 3;
            break;
        }
        case 0x52: {
            // PUSH reg16
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Word regValue = getRegisterByCode16(reg);
            SP -= 2;
            writeToMemory(SP, regValue);
            cycles -= 3;
            break;
        }
        case 0x53: {
            // PUSH ram16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(addr);
            SP -= 2;
            writeToMemory(SP, value);
            cycles -= 6;
            break;
        }
        case 0x54: {
            // POP reg8
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Byte value = readFromMemory(SP);
            setRegisterByCode8(reg, value);
            SP += 1;
            cycles -= 3;
            break;
        }
        case 0x55: {
            // POP reg16
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Word value = readFromMemory(SP);
            setRegisterByCode16(reg, value);
            SP += 2;
            cycles -= 3;
            break;
        }
        case 0x56: {
            // POP ram16
            if (cycles <= 6) return;
            Byte addrLow = fetchNextInstruction();
            Byte addrHigh = fetchNextInstruction();
            Word addr = (addrHigh << 8) | addrLow;
            Word value = readFromMemory(SP);
            writeToMemory(addr, value);
            SP += 2;
            cycles -= 6;
            break;
        }
        case 0x57: {
            // IN reg8
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Byte port = getRegisterByCode8(reg);
            Word value = 0;

            // Read from port
            value = ReadFromPort(port);
            // Save value to AX
            setRegisterByCode16(AXH, value);
            cycles -= 3;
            break;
        }
        case 0x58: {
            // IN reg16
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Byte port = getRegisterByCode8(reg);
            Word value = 0;

            // Read from port
            value = ReadFromPort(port);
            // Save value to AX
            setRegisterByCode16(AXH, value);
            cycles -= 3;
            break;
        }
        case 0x59: {
            // IN imm8
            if (cycles <= 4) return;
            Byte port = fetchNextInstruction();
            Word value = 0;

            // Read from port
            value = ReadFromPort(port);
            // Save value to AX
            setRegisterByCode16(AXH, value);
            cycles -= 4;
            break;
        }
        case 0x5A: {
            // OUT reg8
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Byte port = getRegisterByCode8(reg);
            Word value = getRegisterByCode16(AXH);

            // Write to port
            WriteToPort(port, value);
            cycles -= 3;
            break;
        }
        case 0x5B: {
            // OUT reg16
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Byte port = getRegisterByCode8(reg);
            Word value = getRegisterByCode16(AXH);

            // Write to port
            WriteToPort(port, value);
            cycles -= 3;
            break;
        }
        case 0x5C: {
            // OUT imm8
            if (cycles <= 4) return;
            Byte port = fetchNextInstruction();
            Word value = getRegisterByCode16(AXH);

            // Write to port
            WriteToPort(port, value);
            cycles -= 4;
            break;
        }
        case 0x5D: {
            // INT b8 reg
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Byte regValue = getRegisterByCode8(reg);
            // Call interrupt
            CallInterrupt(regValue);
            cycles -= 5;
            break;
        }
        case 0x5E: {
            // INT b16 reg
            if (cycles <= 3) return;
            Byte reg = fetchNextInstruction();
            Word regValue = getRegisterByCode16(reg);
            // Call interrupt
            CallInterrupt(regValue);
            cycles -= 5;
            break;
        }
        case 0x5F: {
            // INT b8 imm
            if (cycles <= 4) return;
            Byte interrupt = fetchNextInstruction();
            // Call interrupt
            CallInterrupt(interrupt);
            cycles -= 6;
            break;
        }
        case 0x60: {
            // CLI
            if (cycles <= 1) return;
            IF = 0;
            cycles -= 1;
            break;
        }
        case 0x61: {
            // STI
            if (cycles <= 1) return;
            IF = 1;
            cycles -= 1;
            break;
        }
        case 0x62: {
            // CLC
            if (cycles <= 1) return;
            CF = 0;
            cycles -= 1;
            break;
        }
        case 0x63: {
            // STC
            if (cycles <= 1) return;
            CF = 1;
            cycles -= 1;
            break;
        }
        case 0x64: {
            // CLZ
            if (cycles <= 1) return;
            ZF = 0;
            cycles -= 1;
            break;
        }
        case 0x65: {
            // STZ
            if (cycles <= 1) return;
            ZF = 1;
            cycles -= 1;
            break;
        }
        case 0x67: {
            // CLS
            if (cycles <= 1) return;
            SF = 0;
            cycles -= 1;
            break;
        }
        case 0x68: {
            // STS
            if (cycles <= 1) return;
            SF = 1;
            cycles -= 1;
            break;
        }
        case 0x69: {
            // CLD
            if (cycles <= 1) return;
            DF = 0;
            cycles -= 1;
            break;
        }
        case 0x6A: {
            // STD
            if (cycles <= 1) return;
            DF = 1;
            cycles -= 1;
            break;
        }
        case 0x6B: {
            // CLO
            if (cycles <= 1) return;
            OF = 0;
            cycles -= 1;
            break;
        }
        case 0x6C: {
            // STO
            if (cycles <= 1) return;
            OF = 1;
            cycles -= 1;
            break;
        }

        case 0x66: {
            std::cout << "Executed Order 66" << std::endl; // TODO: Remove, TODO: Dont be autistic
            exit(66);
        }

        
        default:
            std::cout << "Invalid opcode" << std::endl;
            break;
        }
    }

    
private:
    AddressBus addressBus;
    DataBus dataBus;
    Memory& memory;

    void writeToMemory(Word segment, Word data) {
        addressBus.setAddress(segment);
        dataBus.setData(data);
        memory.write(addressBus, dataBus);
    }

    Word readFromMemory(Word segment) {
        addressBus.setAddress(segment);
        memory.read(addressBus, dataBus);
        return dataBus.getData();
    }

    Byte fetchNextInstruction() {
        if (cycles <= 0) return -1;
        cycles--;
        Byte opcode = readFromMemory(IP);
        IP++;  // Increment IP to point to the next byte
        return opcode;
    }


    Word getRegisterByCode16(Byte code) {
        switch (code) {
            case AXH: return AX;
            case CXH: return CX;
            case DXH: return DX;
            case BXH: return BX;
            case SPH: return SP;
            case BPH: return BP;
            case SIH: return SI;
            case DIH: return DI;

            default: 
                std::cout << "Invalid register code" << std::endl;
                return 0;
        }
    }

    void setRegisterByCode16(Byte code, Word value) {
        switch (code) {
            case AXH: AX = value; break;
            case CXH: CX = value; break;
            case DXH: DX = value; break;
            case BXH: BX = value; break;
            case SPH: SP = value; break;
            case BPH: BP = value; break;
            case SIH: SI = value; break;
            case DIH: DI = value; break;
            default: 
                std::cout << "Invalid register code" << std::endl;
                break;
        }
    }

    Byte getRegisterByCode8(Byte code) {
        switch (code)
        {
        case AL_CODE: return AL;
        case AH_CODE: return AH;
        case BL_CODE: return BL;
        case BH_CODE: return BH;
        case CL_CODE: return CL;
        case CH_CODE: return CH;
        case DL_CODE: return DL;
        case DH_CODE: return DH;
        
        default:
                std::cout << "Invalid register code" << std::endl;
                return 0;
        }
    }

    void setRegisterByCode8(Byte code, Byte value) {
        switch (code)
        {
        case AL_CODE: AL = value; break;
        case AH_CODE: AH = value; break;
        case BL_CODE: BL = value; break;
        case BH_CODE: BH = value; break;
        case CL_CODE: CL = value; break;
        case CH_CODE: CH = value; break;
        case DL_CODE: DL = value; break;
        case DH_CODE: DH = value; break;
        
        default:
                std::cout << "Invalid register code" << std::endl;
                break;
        }
    }

    void CallInterrupt(Byte interrupt) {
        if (IF == 0) return;
        // Push IP to the stack
        SP -= 2;
        writeToMemory(SP, IP);
        Word handlerAddress = readFromMemory(IDTR + interrupt * 2);
        IP = handlerAddress;
    }
};

int main() {
    Memory mem(1024);
    CPU cpu(mem);
    
    cpu.cycles = 10; // Breakpoint after 10 cycles
    cpu.run();
    
    
        return 0;
}