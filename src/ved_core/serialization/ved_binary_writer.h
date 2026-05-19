#ifndef VED_BINARY_WRITER_H
#define VED_BINARY_WRITER_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "ved_binary_format.h"

class VEDBinaryWriter {
public:
    VEDBinaryWriter() = default;

    [[nodiscard]] const std::vector<std::byte>& Buffer() const noexcept;
    [[nodiscard]] std::span<const std::byte> Bytes() const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] bool Empty() const noexcept;

    void Clear();
    void Reserve(std::size_t size);

    void WriteBytes(std::span<const std::byte> bytes);
    void WriteInt8(std::int8_t value);
    void WriteUInt8(std::uint8_t value);
    void WriteInt16(std::int16_t value);
    void WriteUInt16(std::uint16_t value);
    void WriteInt32(std::int32_t value);
    void WriteUInt32(std::uint32_t value);
    void WriteDouble(double value);
    void WriteBool(bool value);
    void WriteEnum(int value);
    void WriteCString(std::string_view value);
    void WriteFourCC(std::uint32_t value);

private:
    void WriteLittleEndian(std::uint64_t value, std::size_t byteCount);

    std::vector<std::byte> buffer_;
};

#endif
