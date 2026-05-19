#include "ved_binary_reader.h"

#include <algorithm>
#include <bit>
#include <limits>

VEDBinaryReader::VEDBinaryReader(std::span<const std::byte> data)
    : data_(data)
{
}

VEDBinaryReader::VEDBinaryReader(const void* data, std::size_t size)
    : data_(data ? std::span<const std::byte>(static_cast<const std::byte*>(data), size) : std::span<const std::byte>())
    , error_(data || size == 0 ? VEDBinaryError::None : VEDBinaryError::InvalidArgument)
{
}

bool VEDBinaryReader::Ok() const noexcept
{
    return error_ == VEDBinaryError::None;
}

VEDBinaryError VEDBinaryReader::Error() const noexcept
{
    return error_;
}

std::size_t VEDBinaryReader::Position() const noexcept
{
    return position_;
}

std::size_t VEDBinaryReader::Size() const noexcept
{
    return data_.size();
}

std::size_t VEDBinaryReader::Remaining() const noexcept
{
    return data_.size() - position_;
}

bool VEDBinaryReader::End() const noexcept
{
    return position_ == data_.size();
}

void VEDBinaryReader::ClearError() noexcept
{
    error_ = VEDBinaryError::None;
}

void VEDBinaryReader::Fail(VEDBinaryError error) noexcept
{
    if(error_ == VEDBinaryError::None)
        error_ = error;
}

bool VEDBinaryReader::Skip(std::size_t byteCount)
{
    if(!CanRead(byteCount))
        return false;

    position_ += byteCount;
    return true;
}

std::span<const std::byte> VEDBinaryReader::ReadBytes(std::size_t byteCount)
{
    if(!CanRead(byteCount))
        return {};

    const std::span<const std::byte> bytes = data_.subspan(position_, byteCount);
    position_ += byteCount;
    return bytes;
}

std::int8_t VEDBinaryReader::ReadInt8()
{
    return static_cast<std::int8_t>(ReadUInt8());
}

std::uint8_t VEDBinaryReader::ReadUInt8()
{
    return static_cast<std::uint8_t>(ReadLittleEndian(sizeof(std::uint8_t)));
}

std::int16_t VEDBinaryReader::ReadInt16()
{
    return static_cast<std::int16_t>(ReadUInt16());
}

std::uint16_t VEDBinaryReader::ReadUInt16()
{
    return static_cast<std::uint16_t>(ReadLittleEndian(sizeof(std::uint16_t)));
}

std::int32_t VEDBinaryReader::ReadInt32()
{
    return static_cast<std::int32_t>(ReadUInt32());
}

std::uint32_t VEDBinaryReader::ReadUInt32()
{
    return static_cast<std::uint32_t>(ReadLittleEndian(sizeof(std::uint32_t)));
}

double VEDBinaryReader::ReadDouble()
{
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    return std::bit_cast<double>(ReadLittleEndian(sizeof(double)));
}

bool VEDBinaryReader::ReadBool()
{
    return ReadUInt8() != 0;
}

int VEDBinaryReader::ReadEnum()
{
    return static_cast<int>(ReadInt32());
}

std::string VEDBinaryReader::ReadCString()
{
    if(error_ != VEDBinaryError::None)
        return {};

    const auto begin = data_.begin() + static_cast<std::ptrdiff_t>(position_);
    const auto end = data_.end();
    const auto terminator = std::find(begin, end, std::byte{0});
    if(terminator == end) {
        Fail(VEDBinaryError::UnterminatedString);
        return {};
    }

    const std::size_t length = static_cast<std::size_t>(terminator - begin);
    const auto* text = reinterpret_cast<const char*>(data_.data() + position_);
    std::string result(text, length);
    position_ += length + 1U;
    return result;
}

std::uint32_t VEDBinaryReader::ReadFourCC()
{
    return ReadUInt32();
}

bool VEDBinaryReader::CanRead(std::size_t byteCount)
{
    if(error_ != VEDBinaryError::None)
        return false;

    if(byteCount > Remaining()) {
        Fail(VEDBinaryError::EndOfBuffer);
        return false;
    }

    return true;
}

std::uint64_t VEDBinaryReader::ReadLittleEndian(std::size_t byteCount)
{
    if(!CanRead(byteCount))
        return 0U;

    std::uint64_t value = 0;
    for(std::size_t i = 0; i < byteCount; ++i) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(data_[position_ + i])) << (i * std::numeric_limits<unsigned char>::digits);
    }
    position_ += byteCount;
    return value;
}
