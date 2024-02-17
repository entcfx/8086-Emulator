#include "i8086.h"

void i8086::pushByte(Byte value)
{
    SP--;
    writeByte(SP, SS, value);
}

void i8086::pushWord(Word value)
{
    SP -= 2;
    writeByte(SP, SS, value & 0xFF);            // Lower byte
    writeByte(SP + 1, SS, (value >> 8) & 0xFF); // Higher byte
}

Byte i8086::popByte()
{
    Byte value = readByte(SP, SS);
    SP++;
    return value;
}

Word i8086::popWord()
{
    Byte lowByte = readByte(SP, SS);
    Byte highByte = readByte(SP + 1, SS);
    SP += 2;
    return (highByte << 8) | lowByte;
}

Word i8086::getFlags()
{
    return *(Word *)&FR;
}

void i8086::interrupt(Byte vector)
{
    bool isNonMaskable = vector <= 0x1F;
    Word flags = getFlags();
    cycles += 15;

    // Proceed if the interrupt is non-maskable or if the IF is set
    if (isNonMaskable || FR.IF)
    {
        u32 ivtAddress = vector * 4;
        Word isrOffset = readWord(ivtAddress, 0);
        Word isrSegment = readWord(ivtAddress + 2, 0);

        pushWord(flags);
        pushWord(CS);
        pushWord(IP);

        // Clear the IF for maskable interrupts
        if (!isNonMaskable)
        {
            FR.IF = 0;
        }

        CS = isrSegment;
        IP = isrOffset;
    }
}

Byte i8086::fetchByte()
{
    Byte byte = readByte(IP, CS);
    IP += 1;
    return byte;
}

Word i8086::fetchWord()
{
    Word word = readWord(IP, CS);
    IP += 2;
    return word;
}

void i8086::writeWord(u32 address, u32 segment, Word value)
{
    Byte lowByte = value & 0xFF;
    Byte highByte = (value >> 8) & 0xFF;

    writeByte(address, segment, lowByte);
    writeByte(address + 1, segment, highByte);
}

void i8086::writeByte(u32 address, u32 segment, Byte value)
{
    cycles--;
    u32 physicalAddress = (segment * 16) + address;
    if (physicalAddress <= 0xCFFFF)
    {
        ram[physicalAddress] = value;
    }
    else if (physicalAddress >= 0xD0000 && physicalAddress <= 0xFFFFF)
    {
        printf("Error: Writing to ROM is not allowed!\n");
    }
    else
    {
        printf("Error: Trying to access out of bounds memory at %X:%X\n", segment, address);
    }
}

Byte i8086::readByte(u32 address, u32 segment)
{
    cycles--;
    u32 physicalAddress = (segment * 16) + address;
    if (physicalAddress <= 0xCFFFF)
    {
        return ram[physicalAddress];
    }
    else if (physicalAddress >= 0xD0000 && physicalAddress <= 0xFFFFF)
    {
        return rom[physicalAddress - 0xD0000];
    }
    else
    {
        printf("Error: Trying to access out of bounds memory at %X:%X\n", segment, address);
        return 0;
    }
}

Word i8086::readWord(u32 address, u32 segment)
{
    Byte lowByte = readByte(address, segment);
    Byte highByte = readByte(address + 1, segment);
    Word word = (highByte << 8) | lowByte;
    return word;
}

Byte i8086::inBytePort(Word port)
{
    auto it = inPortMap.find(port);
    if (it != inPortMap.end())
    {
        return it->second();
    }
    else
    {
        fprintf(stderr, "Warning: Trying to read from unmapped port \'%x\'", port);
        return 0; // Its always gonna be 0...
    }
}

void i8086::outBytePort(Word port, Byte value)
{
    auto it = outPortMap.find(port);
    if (it != outPortMap.end())
    {
        it->second(value);
    }
    else
    {
        // Handle the case where the port is not mapped
        fprintf(stderr, "Warning: Trying to write to unmapped port \'%x\'", port);
    }
}

bool i8086::execute()
{
}
void i8086::start(u32 cycles)
{
}