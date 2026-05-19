#ifndef VED_BINARY_READER_H
#define VED_BINARY_READER_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "ved_binary_format.h"

class VEDBinaryReader {
public:
    explicit VEDBinaryReader(std::span<const std::byte> data);
    VEDBinaryReader(const void* data, std::size_t size);

    [[nodiscard]] bool Ok() const noexcept;
    [[nodiscard]] VEDBinaryError Error() const noexcept;
    [[nodiscard]] std::size_t Position() const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t Remaining() const noexcept;
    [[nodiscard]] bool End() const noexcept;

    void ClearError() noexcept;
    void Fail(VEDBinaryError error) noexcept;
    bool Skip(std::size_t byteCount);
    std::span<const std::byte> ReadBytes(std::size_t byteCount);

    std::int8_t ReadInt8();
    std::uint8_t ReadUInt8();
    std::int16_t ReadInt16();
    std::uint16_t ReadUInt16();
    std::int32_t ReadInt32();
    std::uint32_t ReadUInt32();
    double ReadDouble();
    bool ReadBool();
    int ReadEnum();
    std::string ReadCString();
    std::uint32_t ReadFourCC();

private:
    bool CanRead(std::size_t byteCount);
    std::uint64_t ReadLittleEndian(std::size_t byteCount);

    std::span<const std::byte> data_;
    std::size_t position_ = 0;
    VEDBinaryError error_ = VEDBinaryError::None;
};

#endif
