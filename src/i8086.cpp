#include "i8086.h"

void i8086::pushByte(Byte value)
{
    SP--;
    cycles -= 2;
    writeByte(SP, SS, value);
}

void i8086::pushWord(Word value)
{
    SP -= 2;
    cycles -= 3;
    writeByte(SP, SS, value & 0xFF);            // Lower byte
    writeByte(SP + 1, SS, (value >> 8) & 0xFF); // Higher byte
}

Byte i8086::popByte()
{
    Byte value = readByte(SP, SS);
    SP++;
    cycles -= 2;
    return value;
}

Word i8086::popWord()
{
    Byte lowByte = readByte(SP, SS);
    Byte highByte = readByte(SP + 1, SS);
    SP += 2;
    cycles -= 3;
    return (highByte << 8) | lowByte;
}

Word i8086::getFlags()
{
    cycles -= 1;
    return *(Word *)&FR;
}

void i8086::interrupt(Byte vector)
{
    bool isNonMaskable = vector <= 0x1F;
    Word flags = getFlags();
    cycles -= 15;

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
    cycles--;
    return byte;
}

Word i8086::fetchWord()
{
    Word word = readWord(IP, CS);
    IP += 2;
    cycles -= 2;
    return word;
}

void i8086::writeWord(u32 address, u32 segment, Word value)
{
    cycles -= 4;
    Byte lowByte = value & 0xFF;
    Byte highByte = (value >> 8) & 0xFF;

    writeByte(address, segment, lowByte);
    writeByte(address + 1, segment, highByte);
}

void i8086::writeByte(u32 address, u32 segment, Byte value)
{
    cycles -= 2;
    u32 physicalAddress = (segment * 16) + address;
    if (physicalAddress <= 0xEFFFF)
    {
        ram[physicalAddress] = value;
    }
    else if (physicalAddress >= 0xF0000 && physicalAddress <= 0xFFFFF)
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
    cycles -= 2;
    u32 physicalAddress = (segment * 16) + address;
    if (physicalAddress <= 0xEFFFF)
    {
        return ram[physicalAddress];
    }
    else if (physicalAddress >= 0xF0000 && physicalAddress <= 0xFFFFF)
    {
        return rom[physicalAddress - 0xF0000];
    }
    else
    {
        printf("Error: Trying to access out of bounds memory at %X:%X\n", segment, address);
        return 0;
    }
}

Word i8086::readWord(u32 address, u32 segment)
{
    cycles -= 4;
    Byte lowByte = readByte(address, segment);
    Byte highByte = readByte(address + 1, segment);
    Word word = (highByte << 8) | lowByte;
    return word;
}

Byte i8086::inBytePort(Word port)
{
    cycles -= 1;
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
    cycles -= 1;
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

Byte i8086::getRegister8Value(Byte regIndex)
{
    switch (regIndex)
    {
    case 0:
        return regs.AL; // AL
    case 1:
        return regs.CL; // CL
    case 2:
        return regs.DL; // DL
    case 3:
        return regs.BL; // BL
    case 4:
        return regs.AH; // AH
    case 5:
        return regs.CH; // CH
    case 6:
        return regs.DH; // DH
    case 7:
        return regs.BH; // BH
    default:
        return 0; // Error case
    }
}
void i8086::setRegister8Value(Byte rmIndex, Byte value)
{
    switch (rmIndex)
    {
    case 0:
        regs.AL = value;
        break; // AL
    case 1:
        regs.CL = value;
        break; // CL
    case 2:
        regs.DL = value;
        break; // DL
    case 3:
        regs.BL = value;
        break; // BL
    case 4:
        regs.AH = value;
        break; // AH
    case 5:
        regs.CH = value;
        break; // CH
    case 6:
        regs.DH = value;
        break; // DH
    case 7:
        regs.BH = value;
        break; // BH
    default:
        break; // Error case
    }
}

void i8086::setSegmentRegister(Byte hexReg, Word value)
{
    switch (hexReg)
    {
    case 0x0:
        CS = value;
        break;
    case 0x1:
        DS = value;
        break;
    case 0x2:
        SS = value;
        break;
    case 0x3:
        ES = value;
        break;
    case 0x4:
        FS = value;
        break;
    case 0x5:
        GS = value;
        break;
    default:
        // Handle error: invalid segment register
        break;
    }
}

Word i8086::getSegmentRegister(Byte hexReg)
{
    switch (hexReg)
    {
    case 0x0:
        return CS;
    case 0x1:
        return DS;
    case 0x2:
        return SS;
    case 0x3:
        return ES;
    case 0x4:
        return FS;
    case 0x5:
        return GS;
    default:
        // Handle error: invalid segment register
        return 0; // or throw an exception
    }
}

bool i8086::isMemoryOperand(Byte modRM)
{
    Byte mod = (modRM >> 6) & 0x03;
    return mod != 0x03; // Memory operand if mod isn't 0x03
}

bool i8086::execute()
{
    if (FR.TF)
    {
        interrupt(1);
        cycles -= 50;
    }

    // TODO Implement Interrupt check once I make a PIC

    handleRepe(); // Will handle REPE and override segments
    exeOpcode();  // Will fetch and execute one opcode
}

void i8086::setRegister16Value(Byte regIndex, Word value)
{
    switch (regIndex)
    {
    case 0: // AX
        regs.AX = value;
        break;
    case 1: // BX
        regs.BX = value;
        break;
    case 2: // CX
        regs.CX = value;
        break;
    case 3: // DX
        regs.DX = value;
        break;
    default:
        // Handle invalid register index
        break;
    }
}

u32 i8086::getAddressFromModRM(Byte modRM, Word segment)
{
    Byte mod = modRM >> 6; // First two bits
    Byte rm = modRM & 0x7; // Last three bits
    u32 address = 0;

    switch (mod)
    {
    case 0:               // Register addressing mode
        address = rm * 2; // Assuming consecutive register indexes
        break;
    case 1: // Addressing mode with displacement byte
        address = fetchByte();
        break;
    case 2: // Addressing mode with displacement word
        address = fetchWord();
        break;
    case 3: // Register addressing mode
        // Not applicable for this function
        break;
    default:
        // Handle invalid mod
        break;
    }

    return address;
}

void i8086::handleRepe()
{
    os = &DS; // Default segment register
    bool hasRepPrefix = false;
    bool repne = false; // REPNE/REPNZ prefix flag

    Byte prefix = fetchByte();
    while (true) // Loop to handle multiple prefixes
    {
        switch (prefix)
        {
        case 0x26:
            os = &ES;
            break; // ES segment override
        case 0x2E:
            os = &CS;
            break; // CS segment override
        case 0x36:
            os = &SS;
            break; // SS segment override
        case 0x3E:
            os = &DS;
            break; // DS segment override
        case 0xF2: // REPNE/REPNZ prefix
            hasRepPrefix = true;
            repne = true;
            break;
        case 0xF3: // REP or REPE/REPZ prefix
            hasRepPrefix = true;
            repne = false;
            break;
        default:
            IP--;                 // Adjust IP to re-read the byte as an opcode
            goto end_prefix_loop; // Exit the loop
        }
        prefix = fetchByte(); // Fetch the next byte to check for additional prefixes
    }
end_prefix_loop:

    if (hasRepPrefix)
    {
        executeStringInstruction(repne); // Execute the string instruction with repeat logic
    }
}
void i8086::executeStringInstruction(bool repne)
{
    Byte opcode = fetchByte(); // Fetch the string operation opcode

    // Execute the string operation in a loop
    while (regs.CX != 0)
    {
        switch (opcode)
        {
        case 0xA4:
            movsb(os);
            break; // MOVSB
        case 0xA5:
            movsw(os);
            break; // MOVSW
        case 0xAA:
            stosb(os);
            break; // STOSB
        case 0xAB:
            stosw(os);
            break; // STOSW
        case 0xAC:
            lodsb(os);
            break; // LODSB
        case 0xAD:
            lodsw(os);
            break; // LODSW
        case 0xAE:
            scasb(os);
            break; // SCASB
        case 0xAF:
            scasw(os);
            break; // SCASW
        default:
            // Handle unexpected opcode
            break;
        }

        regs.CX--;

        // For REPNE/REPE, also check the Zero Flag condition
        if (repne && FR.ZF == 1)
            break; // REPNE and ZF is set, exit loop
        if (!repne && FR.ZF == 0)
            break; // REPE and ZF is clear, exit loop
    }
}
void i8086::movsb(Word *segmentOverride)
{
    Word segment = segmentOverride ? *segmentOverride : DS; // Use the override segment or DS by default
    Byte value = readByte(SI, segment);
    writeByte(DI, ES, value);    // Always use ES for the destination in string operations
    SI += (FR.DF == 0) ? 1 : -1; // Update SI based on the direction flag
    DI += (FR.DF == 0) ? 1 : -1; // Update DI similarly
}
void i8086::movsw(Word *segmentOverride)
{
    Word segment = segmentOverride ? *segmentOverride : DS;
    Word value = readWord(SI, segment);
    writeWord(DI, ES, value);    // Always use ES for the destination in string operations
    SI += (FR.DF == 0) ? 2 : -2; // Update SI based on the direction flag
    DI += (FR.DF == 0) ? 2 : -2; // Update DI similarly
}
void i8086::stosb(Word *segmentOverride)
{
    writeByte(DI, ES, regs.AL);  // Store AL at [ES:DI]
    DI += (FR.DF == 0) ? 1 : -1; // Update DI based on the direction flag
}
void i8086::stosw(Word *segmentOverride)
{
    writeWord(DI, ES, regs.AX);  // Store AX at [ES:DI]
    DI += (FR.DF == 0) ? 2 : -2; // Update DI based on the direction flag
}
void i8086::lodsb(Word *segmentOverride)
{
    Word segment = segmentOverride ? *segmentOverride : DS;
    regs.AL = readByte(SI, segment); // Load byte at [DS:SI] into AL
    SI += (FR.DF == 0) ? 1 : -1;     // Update SI based on the direction flag
}
void i8086::lodsw(Word *segmentOverride)
{
    Word segment = segmentOverride ? *segmentOverride : DS;
    regs.AX = readWord(SI, segment); // Load word at [DS:SI] into AX
    SI += (FR.DF == 0) ? 2 : -2;     // Update SI based on the direction flag
}
void i8086::scasb(Word *segmentOverride)
{
    Byte value = readByte(DI, ES); // Always use ES for destination in SCAS operations
    Byte result = regs.AL - value; // Perform subtraction to compare

    FR.ZF = (result == 0) ? 1 : 0; // Set ZF if result is zero (values are equal)

    DI += (FR.DF == 0) ? 1 : -1; // Update DI based on the direction flag
}
void i8086::scasw(Word *segmentOverride)
{
    Word value = readWord(DI, ES); // Always use ES for destination in SCAS operations
    Word result = regs.AX - value; // Perform subtraction to compare

    // Update flags based on the result
    FR.ZF = (result == 0) ? 1 : 0; // Set ZF if result is zero (values are equal)

    DI += (FR.DF == 0) ? 2 : -2; // Update DI based on the direction flag
}

void i8086::exeOpcode()
{
    Byte opcode = fetchByte(); // Fetch the opcode

    switch (opcode)
    {
    case 0x88: // mov reg8/mem8,reg8
    case 0x89: // mov reg16/mem16,reg16
    case 0x8a: // mov reg8,reg8/mem8
    case 0x8b: // mov reg16,reg16/mem16
    {          // Decode the instruction
        Byte modRM = fetchByte();

        Byte mod = modRM >> 6;         // First two bits
        Byte reg = (modRM >> 3) & 0x7; // Middle three bits
        Byte rm = modRM & 0x7;         // Last three bits

        Word operand;
        Word result;

        if (mod == 0b11) // Register to Register
        {
            if ((opcode == 0x88 || opcode == 0x8a) && (reg == 0b000 || reg == 0b001)) // mov reg8, reg8/mem8
            {
                operand = getRegister8Value(rm);
                setRegister8Value(reg, operand);
            }
            else // mov reg16, reg16/mem16
            {
                operand = regs.AX + (reg * 2); // Assuming consecutive registers in memory
                result = readWord(operand, DS);
                setRegister16Value(reg, operand);
            }
        }
        else // Memory to/from Register
        {
            u32 address = getAddressFromModRM(modRM, DS);                             // Assuming DS segment
            if ((opcode == 0x88 || opcode == 0x8a) && (reg == 0b000 || reg == 0b001)) // mov reg8/mem8, reg8
            {
                operand = readByte(address, DS);
                setRegister8Value(reg, operand);
            }
            else // mov reg16/mem16, reg16
            {
                operand = readWord(address, DS);
                setRegister16Value(reg, operand);
            }
        }

        // Update cycle count
        cycles += mod == 0b11 ? 2 : 9; // Assuming direct register addressing
        break;
    }

    case 0xc6: // mov reg8/mem8,immed8
    case 0xc7: // mov reg16/mem16,immed16
    {
        // Decode the instruction
        Byte modRM = fetchByte();
        Byte mod = modRM >> 6;         // First two bits
        Byte reg = (modRM >> 3) & 0x7; // Middle three bits
        Byte rm = modRM & 0x7;         // Last three bits

        Word address = 0;
        Byte immByte = 0;
        Word immWord = 0;

        // Determine the size of the immediate value based on the opcode
        bool isImmediate16 = (opcode == 0xc7);

        if (isImmediate16) // Immediate 16-bit value
        {
            immWord = fetchWord();
        }
        else // Immediate 8-bit value
        {
            immByte = fetchByte();
        }

        if (mod == 0b11) // Register addressing mode
        {
            if (isImmediate16)
            {
                setRegister16Value(reg, immWord);
            }
            else
            {
                setRegister8Value(reg, immByte);
            }
        }
        else // Memory addressing mode
        {
            // Get the memory address
            address = getAddressFromModRM(modRM, DS); // Assuming DS segment

            // Write the immediate value to memory
            if (isImmediate16)
            {
                writeWord(address, DS, immWord);
            }
            else
            {
                writeByte(address, DS, immByte);
            }
        }

        // Update cycle count
        cycles += isImmediate16 ? 4 : 10; // Assuming immediate values
        break;
    }

    case 0xb0: // mov al,immed8
    case 0xb1: // mov cl,immed8
    case 0xb2: // mov dl,immed8
    case 0xb3: // mov bl,immed8
    case 0xb4: // mov ah,immed8
    case 0xb5: // mov ch,immed8
    case 0xb6: // mov dh,immed8
    case 0xb7: // mov bh,immed8
    {
        // Decode the instruction
        Byte regIndex = opcode - 0xb0; // Calculate the register index
        Byte immByte = fetchByte();    // Fetch the immediate byte value

        // Set the immediate byte value into the appropriate register
        setRegister8Value(regIndex, immByte);

        // Update cycle count
        cycles += 4; // Assuming immediate value
        break;
    }
    case 0xb8: // mov ax,immed16
    case 0xb9: // mov cx,immed16
    case 0xba: // mov dx,immed16
    case 0xbb: // mov bx,immed16
    case 0xbc: // mov sp,immed16
    case 0xbd: // mov bp,immed16
    case 0xbe: // mov si,immed16
    case 0xbf: // mov di,immed16
    {
        // Decode the instruction
        Byte regIndex = opcode - 0xb8; // Calculate the register index
        Word immWord = fetchWord();    // Fetch the immediate word value

        // Set the immediate word value into the appropriate register
        setRegister16Value(regIndex, immWord);

        // Update cycle count
        cycles += 4; // Assuming immediate value
        break;
    }

    case 0xa0: // mov al,mem8
    {
        // Fetch the immediate address from the instruction
        Word address = fetchWord();

        // Read the byte from memory at the specified address and store it in AL
        regs.AL = readByte(address, DS);

        // Update cycle count
        cycles += 10; // Assuming memory access
        break;
    }
    case 0xa1: // mov ax,mem16
    {
        // Fetch the immediate address from the instruction
        Word address = fetchWord();

        // Read the word from memory at the specified address and store it in AX
        regs.AX = readWord(address, DS);

        // Update cycle count
        cycles += 10; // Assuming memory access
        break;
    }
    case 0xa2: // mov mem8,al
    {
        // Fetch the immediate address from the instruction
        Word address = fetchWord();

        // Write the byte stored in AL to memory at the specified address
        writeByte(address, DS, regs.AL);

        // Update cycle count
        cycles += 10; // Assuming memory access
        break;
    }
    case 0xa3: // mov mem16,ax
    {
        // Fetch the immediate address from the instruction
        Word address = fetchWord();

        // Write the word stored in AX to memory at the specified address
        writeWord(address, DS, regs.AX);

        // Update cycle count
        cycles += 10; // Assuming memory access
        break;
    }

    case 0x8c: // mov reg16/mem16,segreg
    {
        // Decode the instruction
        Byte modRM = fetchByte();

        Byte mod = modRM >> 6;         // First two bits
        Byte reg = (modRM >> 3) & 0x7; // Middle three bits
        Byte rm = modRM & 0x7;         // Last three bits

        Word operand;

        if (mod == 0b11) // Register to Register
        {
            // Get the value of the segment register
            operand = getSegmentRegister(reg);

            // Set the value to the destination register or memory
            writeWord(getAddressFromModRM(modRM, DS), DS, operand);

            // Update cycle count
            cycles += mod == 0b11 ? 2 : 9; // Assuming direct register addressing
        }
        else // Memory to Segment Register
        {
            // Get the address from the ModRM byte
            u32 address = getAddressFromModRM(modRM, DS); // Assuming DS segment

            // Read the word from memory at the specified address
            operand = readWord(address, DS);

            // Set the value of the segment register
            setSegmentRegister(reg, operand);

            // Update cycle count
            cycles += mod == 0b11 ? 2 : 9; // Assuming memory access
        }

        break;
    }
    case 0x8e: // mov segreg,reg16/mem16
    {
        // Decode the instruction
        Byte modRM = fetchByte();

        Byte mod = modRM >> 6;         // First two bits
        Byte reg = (modRM >> 3) & 0x7; // Middle three bits
        Byte rm = modRM & 0x7;         // Last three bits

        Word operand;

        if (mod == 0b11) // Register to Register
        {
            // Get the value of the source register or memory
            operand = readWord(getAddressFromModRM(modRM, DS), DS);

            // Set the value to the segment register
            setSegmentRegister(reg, operand);

            // Update cycle count
            cycles += mod == 0b11 ? 2 : 9; // Assuming direct register addressing
        }
        else // Segment Register to Memory
        {
            // Get the address from the ModRM byte
            u32 address = getAddressFromModRM(modRM, DS); // Assuming DS segment

            // Get the value of the segment register
            operand = getSegmentRegister(reg);

            // Write the word to memory at the specified address
            writeWord(address, DS, operand);

            // Update cycle count
            cycles += mod == 0b11 ? 2 : 9; // Assuming memory access
        }

        break;
    }

        //

        //

        //

        //

        //

    default:
        // Handle unknown opcodes
        break;
    }
}

void i8086::init()
{
    IP = 0xFFFF0;
}

void i8086::start(u32 cycles)
{
}