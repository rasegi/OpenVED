#ifndef VED_BINARY_FORMAT_H
#define VED_BINARY_FORMAT_H

#include <cstdint>

enum class VEDBinaryError {
    None,
    EndOfBuffer,
    UnterminatedString,
    InvalidArgument,
    InvalidSignature,
    UnsupportedVersion,
    UnsupportedObjectType,
    UnknownObjectType
};

constexpr std::uint32_t VEDMakeFourCC(char a, char b, char c, char d)
{
    return static_cast<std::uint32_t>(static_cast<unsigned char>(a)) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 8U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 16U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(d)) << 24U);
}

#endif
