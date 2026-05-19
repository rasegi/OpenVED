#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_polycurve.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace {

std::uint8_t byteAt(std::span<const std::byte> bytes, std::size_t index)
{
    return std::to_integer<std::uint8_t>(bytes[index]);
}

} // namespace

int main()
{
    VEDBinaryWriter writer;
    writer.WriteInt8(-5);
    writer.WriteUInt8(250U);
    writer.WriteInt16(-1234);
    writer.WriteUInt16(0x1234U);
    writer.WriteInt32(-1234567);
    writer.WriteUInt32(0x89abcdefU);
    writer.WriteFourCC(VEDMakeFourCC('v', 'f', 'n', 't'));
    writer.WriteDouble(12.5);
    writer.WriteBool(false);
    writer.WriteBool(true);
    writer.WriteEnum(-17);
    writer.WriteCString("VED");
    writer.WriteCString("");

    const std::span<const std::byte> bytes = writer.Bytes();
    assert(bytes.size() == 1U + 1U + 2U + 2U + 4U + 4U + 4U + 8U + 1U + 1U + 4U + 4U + 1U);
    assert(byteAt(bytes, 0) == 0xfbU);
    assert(byteAt(bytes, 1) == 0xfaU);
    assert(byteAt(bytes, 2) == 0x2eU);
    assert(byteAt(bytes, 3) == 0xfbU);
    assert(byteAt(bytes, 4) == 0x34U);
    assert(byteAt(bytes, 5) == 0x12U);
    assert(byteAt(bytes, 6) == 0x79U);
    assert(byteAt(bytes, 7) == 0x29U);
    assert(byteAt(bytes, 8) == 0xedU);
    assert(byteAt(bytes, 9) == 0xffU);
    assert(byteAt(bytes, 10) == 0xefU);
    assert(byteAt(bytes, 11) == 0xcdU);
    assert(byteAt(bytes, 12) == 0xabU);
    assert(byteAt(bytes, 13) == 0x89U);
    assert(byteAt(bytes, 14) == 'v');
    assert(byteAt(bytes, 15) == 'f');
    assert(byteAt(bytes, 16) == 'n');
    assert(byteAt(bytes, 17) == 't');

    VEDBinaryReader reader(bytes);
    assert(reader.ReadInt8() == -5);
    assert(reader.ReadUInt8() == 250U);
    assert(reader.ReadInt16() == -1234);
    assert(reader.ReadUInt16() == 0x1234U);
    assert(reader.ReadInt32() == -1234567);
    assert(reader.ReadUInt32() == 0x89abcdefU);
    assert(reader.ReadFourCC() == VEDMakeFourCC('v', 'f', 'n', 't'));
    assert(std::fabs(reader.ReadDouble() - 12.5) < 0.000001);
    assert(!reader.ReadBool());
    assert(reader.ReadBool());
    assert(reader.ReadEnum() == -17);
    assert(reader.ReadCString() == "VED");
    assert(reader.ReadCString().empty());
    assert(reader.Ok());
    assert(reader.End());

    const std::array<std::byte, 1> shortBuffer = {std::byte{0x01}};
    VEDBinaryReader shortReader(shortBuffer);
    assert(shortReader.ReadUInt16() == 0U);
    assert(shortReader.Error() == VEDBinaryError::EndOfBuffer);
    assert(shortReader.Position() == 0U);

    const std::array<std::byte, 2> unterminatedString = {std::byte{'V'}, std::byte{'E'}};
    VEDBinaryReader stringReader(unterminatedString);
    assert(stringReader.ReadCString().empty());
    assert(stringReader.Error() == VEDBinaryError::UnterminatedString);
    assert(stringReader.Position() == 0U);

    const std::array<std::byte, 3> rawBytes = {std::byte{0xaa}, std::byte{0xbb}, std::byte{0xcc}};
    VEDBinaryReader rawReader(rawBytes.data(), rawBytes.size());
    const std::span<const std::byte> firstTwo = rawReader.ReadBytes(2);
    assert(firstTwo.size() == 2U);
    assert(byteAt(firstTwo, 0) == 0xaaU);
    assert(byteAt(firstTwo, 1) == 0xbbU);
    assert(rawReader.Remaining() == 1U);
    assert(rawReader.Skip(1));
    assert(rawReader.End());
    assert(rawReader.Ok());

    TDVecPolyCurve curve;
    curve.SetResolution(7);
    curve.SetShowControls(false);
    curve.SetShowPolygon(true);
    curve.AppendPoint({1.0, 2.0, CPT_PRIME_LINE, true});
    curve.AppendPoint({3.0, 4.0, CPT_PRIME_QSPLINE, false});

    VEDBinaryWriter curveWriter;
    curveWriter.WriteFourCC(curve.TypeFourCC());
    curve.WriteTo(curveWriter);

    VEDBinaryReader curveReader(curveWriter.Bytes());
    assert(curveReader.ReadFourCC() == TDVecPolyCurve::StreamFourCC());
    std::unique_ptr<TDVecPolyCurve> readCurve = TDVecPolyCurve::ReadFrom(curveReader);
    assert(readCurve);
    assert(readCurve->GetType() == VECOBJ_POLYCURVE);
    assert(readCurve->GetResolution() == 7U);
    assert(!readCurve->GetShowControls());
    assert(readCurve->GetShowPolygon());
    assert(readCurve->CountPoints() == 2);
    const TDMatConturPoint* point = readCurve->GetPoint(1);
    assert(point);
    assert(std::fabs(point->x - 3.0) < 0.000001);
    assert(std::fabs(point->y - 4.0) < 0.000001);
    assert(point->eType == CPT_PRIME_QSPLINE);
    assert(!point->bJoint);
    assert(curveReader.Ok());

    return 0;
}
