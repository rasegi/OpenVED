#include "ved_binary_writer.h"

#include <bit>
#include <cstring>
#include <limits>

const std::vector<std::byte>& VEDBinaryWriter::Buffer() const noexcept
{
    return buffer_;
}

std::span<const std::byte> VEDBinaryWriter::Bytes() const noexcept
{
    return std::span<const std::byte>(buffer_.data(), buffer_.size());
}

std::size_t VEDBinaryWriter::Size() const noexcept
{
    return buffer_.size();
}

bool VEDBinaryWriter::Empty() const noexcept
{
    return buffer_.empty();
}

void VEDBinaryWriter::Clear()
{
    buffer_.clear();
}

void VEDBinaryWriter::Reserve(std::size_t size)
{
    buffer_.reserve(size);
}

void VEDBinaryWriter::WriteBytes(std::span<const std::byte> bytes)
{
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

void VEDBinaryWriter::WriteInt8(std::int8_t value)
{
    WriteUInt8(static_cast<std::uint8_t>(value));
}

void VEDBinaryWriter::WriteUInt8(std::uint8_t value)
{
    buffer_.push_back(static_cast<std::byte>(value));
}

void VEDBinaryWriter::WriteInt16(std::int16_t value)
{
    WriteUInt16(static_cast<std::uint16_t>(value));
}

void VEDBinaryWriter::WriteUInt16(std::uint16_t value)
{
    WriteLittleEndian(value, sizeof(value));
}

void VEDBinaryWriter::WriteInt32(std::int32_t value)
{
    WriteUInt32(static_cast<std::uint32_t>(value));
}

void VEDBinaryWriter::WriteUInt32(std::uint32_t value)
{
    WriteLittleEndian(value, sizeof(value));
}

void VEDBinaryWriter::WriteDouble(double value)
{
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    WriteLittleEndian(std::bit_cast<std::uint64_t>(value), sizeof(value));
}

void VEDBinaryWriter::WriteBool(bool value)
{
    WriteUInt8(value ? 1U : 0U);
}

void VEDBinaryWriter::WriteEnum(int value)
{
    WriteInt32(static_cast<std::int32_t>(value));
}

void VEDBinaryWriter::WriteCString(std::string_view value)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(value.data());
    WriteBytes(std::span<const std::byte>(bytes, value.size()));
    WriteUInt8(0U);
}

void VEDBinaryWriter::WriteFourCC(std::uint32_t value)
{
    WriteUInt32(value);
}

void VEDBinaryWriter::WriteLittleEndian(std::uint64_t value, std::size_t byteCount)
{
    for(std::size_t i = 0; i < byteCount; ++i) {
        buffer_.push_back(static_cast<std::byte>((value >> (i * std::numeric_limits<unsigned char>::digits)) & 0xffU));
    }
}
