#include "header.h"

// 1 MiB RAM
#define MEM_SIZE 1024 * 1024

class memory
{
public:
    Byte data[MEM_SIZE]; // Memory array

    Byte &operator[](Byte index)
    {
        if (index >= MEM_SIZE)
        {
            printf("Memory access error: address %x out of range\n", index);
            return data[0];
        }
        return data[index];
    }

    // Const version of operator[] for read-only access
    const Byte &operator[](Byte index) const
    {
        if (index >= MEM_SIZE)
        {
            printf("Constant memory access error: address %x out of range\n", index);
            return data[0];
        }
        return data[index];
    }
};