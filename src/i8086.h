#pragma once
#include "header.h"
#include "ram.hpp"

class i8086
{
public:
    Word IP;
    union GPReg
    {
        struct
        {
            Word AX;
            Word BX;
            Word CX;
            Word DX;
        };
        struct
        {
            Byte AL;
            Byte AH;
            Byte BL;
            Byte BH;
            Byte CL;
            Byte CH;
            Byte DL;
            Byte DH;
        };
    };
    Word SI, DI;
    Word SP, BP;
    Word CS, SS, DS, ES, FS, GS;

    struct Flags
    {                       // Flags register, can be pushed
        Word CF : 1;        // Carry Flag, bit 0
        Word reserved1 : 1; // Reserved, always 1, bit 1
        Word PF : 1;        // Parity Flag, bit 2
        Word reserved2 : 1; // Reserved, bit 3
        Word AF : 1;        // Auxiliary Carry Flag, bit 4
        Word reserved3 : 1; // Reserved, bit 5
        Word ZF : 1;        // Zero Flag, bit 6
        Word SF : 1;        // Sign Flag, bit 7
        Word TF : 1;        // Trap Flag (Single Step), bit 8
        Word IF : 1;        // Interrupt Enable Flag, bit 9
        Word DF : 1;        // Direction Flag, bit 10
        Word OF : 1;        // Overflow Flag, bit 11
        Word IOPL : 2;      // I/O Privilege Level (unused in 8086, relevant in later models), bits 12-13
        Word NT : 1;        // Nested Task Flag (unused in 8086, relevant in later models), bit 14
        Word reserved4 : 1; // Reserved, bit 15
    };
    Flags FR;
    GPReg regs;

    memory ram; // 0x00000 -> 0xCFFFF
    memory rom; // 0x 0xF0000 -> 0xFFFFF

    Byte readByte(u32 address, u32 segment);
    Word readWord(u32 address, u32 segment);
    void writeByte(u32 address, u32 segment, Byte value);
    void writeWord(u32 address, u32 segment, Word value);
    Word fetchWord();
    Byte fetchByte();

    bool execute();
    void start(u32 cycles);
    void init();

    void interrupt(Byte vector);

    Byte inBytePort(Word port);
    void outBytePort(Word port, Byte value);

    /* Public so the IO devices can write to it without a helper function */
    std::unordered_map<Word, InPortFunction> inPortMap;
    std::unordered_map<Word, OutPortFunction> outPortMap;

private:
    u32 cycles;
    bool halt;
    Word *os;

    Word
    getFlags();
    void pushByte(Byte value);
    void pushWord(Word value);
    Byte popByte();
    Word popWord();

    Byte getRegister8Value(Byte regIndex);
    bool isMemoryOperand(Byte modRM);
    void setRegister8Value(Byte rmIndex, Byte value);
    void handleRepe();
    void exeOpcode();
    void executeStringInstruction(bool repne);
    void setRegister16Value(Byte regIndex, Word value);
    u32 getAddressFromModRM(Byte modRM, Word segment);
    void setSegmentRegister(Byte hexReg, Word value);
    Word getSegmentRegister(Byte hexReg);

    void movsb(Word *segmentOverride);
    void movsw(Word *segmentOverride);
    void stosb(Word *segmentOverride);
    void stosw(Word *segmentOverride);
    void lodsw(Word *segmentOverride);
    void lodsb(Word *segmentOverride);
    void scasb(Word *segmentOverride);
    void scasw(Word *segmentOverride);
};