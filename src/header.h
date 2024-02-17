#pragma once
#include <stdio.h>

#include <functional>
#include <unordered_map>

using Byte = unsigned char;
using Word = unsigned short;

using u32 = unsigned int;

using InPortFunction = std::function<Byte()>;
using OutPortFunction = std::function<void(Byte)>;