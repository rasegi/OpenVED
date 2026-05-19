#include "vec_font.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace {

void copyFixedString(char* destination, size_t destinationSize, const char* source)
{
    if (!destination || destinationSize == 0) {
        return;
    }
    std::snprintf(destination, destinationSize, "%s", source ? source : "");
}

template <class T>
T* insertOwned(std::vector<std::unique_ptr<T>>& items, int index, T* object)
{
    if (index < 0 || index > static_cast<int>(items.size())) {
        return nullptr;
    }
    items.insert(items.begin() + index, std::unique_ptr<T>(object));
    return object;
}

template <class T>
T* appendOwned(std::vector<std::unique_ptr<T>>& items, T* object)
{
    items.push_back(std::unique_ptr<T>(object));
    return object;
}

template <class T>
bool removeOwned(std::vector<std::unique_ptr<T>>& items, int index)
{
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return false;
    }
    items.erase(items.begin() + index);
    return true;
}

template <class T>
bool deleteOwned(std::vector<std::unique_ptr<T>>& items, int index)
{
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return false;
    }
    items[index].reset();
    return true;
}

template <class T>
T* dropOwned(std::vector<std::unique_ptr<T>>& items, int index)
{
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return nullptr;
    }
    std::unique_ptr<T> object = std::move(items[index]);
    items.erase(items.begin() + index);
    return object.release();
}

template <class T>
T* getOwned(const std::vector<std::unique_ptr<T>>& items, int index)
{
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return nullptr;
    }
    return items[index].get();
}

template <class T>
void compactOwned(std::vector<std::unique_ptr<T>>& items)
{
    items.erase(std::remove_if(items.begin(), items.end(), [](const std::unique_ptr<T>& item) {
        return item == nullptr;
    }), items.end());
}

template <class T>
void writeObjectVector(VEDBinaryWriter& writer, const std::vector<std::unique_ptr<T>>& items)
{
    writer.WriteInt32(static_cast<std::int32_t>(items.size()));
    for(const std::unique_ptr<T>& item : items) {
        writer.WriteFourCC(T::StreamFourCC());
        item->WriteTo(writer);
    }
}

template <class T>
void readObjectVector(VEDBinaryReader& reader, std::vector<std::unique_ptr<T>>& items)
{
    items.clear();
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return;
    }
    items.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        const std::uint32_t signature = reader.ReadFourCC();
        if(signature != T::StreamFourCC()) {
            reader.Fail(VEDBinaryError::InvalidSignature);
            return;
        }
        std::unique_ptr<T> item = T::ReadFrom(reader);
        if(!item) {
            return;
        }
        items.push_back(std::move(item));
    }
}

void moveLine(TDMatLine& line, double dx, double dy)
{
    line.x1 += dx;
    line.y1 += dy;
    line.x2 += dx;
    line.y2 += dy;
}

void rotateLine(TDMatLine& line, TDMatPoint origin, double angle)
{
    const TDMatPoint p1{line.x1 - origin.x, line.y1 - origin.y};
    const TDMatPoint p2{line.x2 - origin.x, line.y2 - origin.y};
    line.x1 = cosD(angle) * p1.x - sinD(angle) * p1.y + origin.x;
    line.y1 = sinD(angle) * p1.x + cosD(angle) * p1.y + origin.y;
    line.x2 = cosD(angle) * p2.x - sinD(angle) * p2.y + origin.x;
    line.y2 = sinD(angle) * p2.x + cosD(angle) * p2.y + origin.y;
}

void transformLineToPoint(TDMatLine& line, TDMatPoint point)
{
    line.x1 -= point.x;
    line.y1 -= point.y;
    line.x2 -= point.x;
    line.y2 -= point.y;
}

void transformLineToOrigin(TDMatLine& line, TDMatPoint point)
{
    line.x1 += point.x;
    line.y1 += point.y;
    line.x2 += point.x;
    line.y2 += point.y;
}

void scaleLine(TDMatLine& line, TDMatPoint origin, double xScale, double yScale)
{
    transformLineToPoint(line, origin);
    line.x1 *= xScale;
    line.y1 *= yScale;
    line.x2 *= xScale;
    line.y2 *= yScale;
    transformLineToOrigin(line, origin);
}

TDMatRect emptyRect()
{
    return {};
}

void expandFrame(TDMatRect& frame, const TDMatRect& next, bool& initialized)
{
    if (!initialized) {
        frame = next;
        initialized = true;
        return;
    }

    const double left = std::min({frame.P1.x, frame.P2.x, frame.P3.x, frame.P4.x, next.P1.x, next.P2.x, next.P3.x, next.P4.x});
    const double right = std::max({frame.P1.x, frame.P2.x, frame.P3.x, frame.P4.x, next.P1.x, next.P2.x, next.P3.x, next.P4.x});
    const double top = std::min({frame.P1.y, frame.P2.y, frame.P3.y, frame.P4.y, next.P1.y, next.P2.y, next.P3.y, next.P4.y});
    const double bottom = std::max({frame.P1.y, frame.P2.y, frame.P3.y, frame.P4.y, next.P1.y, next.P2.y, next.P3.y, next.P4.y});
    frame.P1 = {left, top};
    frame.P2 = {right, top};
    frame.P3 = {right, bottom};
    frame.P4 = {left, bottom};
}

} // namespace

TDVecGlyph::TDVecGlyph()
    : mnCharWidth(0.0)
{
    mBaseLine.InitialWithNULL();
}

TDVecGlyph::TDVecGlyph(const TDVecGlyph& glyph)
    : mBaseLine(glyph.mBaseLine),
      mnCharWidth(glyph.mnCharWidth)
{
    for (int i = 0; i < glyph.CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = glyph.GetPolyCurve(i);
        polyCurves_.push_back(curve ? std::make_unique<TDVecPolyCurve>(*curve) : nullptr);
    }
}

TDVecGlyph::~TDVecGlyph()
{
}

TDVecGlyph& TDVecGlyph::operator=(const TDVecGlyph& glyph)
{
    if (this == &glyph) {
        return *this;
    }
    mBaseLine = glyph.mBaseLine;
    mnCharWidth = glyph.mnCharWidth;
    polyCurves_.clear();
    for (int i = 0; i < glyph.CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = glyph.GetPolyCurve(i);
        polyCurves_.push_back(curve ? std::make_unique<TDVecPolyCurve>(*curve) : nullptr);
    }
    return *this;
}

TDVecGlyph* TDVecGlyph::Clone() const
{
    return new TDVecGlyph(*this);
}

std::uint32_t TDVecGlyph::StreamFourCC()
{
    return VEDMakeFourCC('v', 'g', 'l', 'y');
}

std::unique_ptr<TDVecGlyph> TDVecGlyph::ReadFrom(VEDBinaryReader& reader)
{
    auto glyph = std::make_unique<TDVecGlyph>();
    glyph->mnCharWidth = reader.ReadDouble();
    glyph->mBaseLine.x1 = reader.ReadDouble();
    glyph->mBaseLine.y1 = reader.ReadDouble();
    glyph->mBaseLine.x2 = reader.ReadDouble();
    glyph->mBaseLine.y2 = reader.ReadDouble();
    readObjectVector(reader, glyph->polyCurves_);
    return reader.Ok() ? std::move(glyph) : nullptr;
}

void TDVecGlyph::WriteTo(VEDBinaryWriter& writer) const
{
    writer.WriteDouble(mnCharWidth);
    writer.WriteDouble(mBaseLine.x1);
    writer.WriteDouble(mBaseLine.y1);
    writer.WriteDouble(mBaseLine.x2);
    writer.WriteDouble(mBaseLine.y2);
    writeObjectVector(writer, polyCurves_);
}

bool TDVecGlyph::IsSpacing() const
{
    return polyCurves_.empty();
}

void TDVecGlyph::SetBaseLine(double nAscent)
{
    const TDMatRect rect = GetFrame();
    mBaseLine.Create({rect.P1.x, nAscent}, {rect.P2.x, nAscent});
}

TDMatLine TDVecGlyph::GetBaseLine() const
{
    return mBaseLine;
}

void TDVecGlyph::SetCharWidth(double nCharWidth)
{
    mnCharWidth = nCharWidth;
}

double TDVecGlyph::GetCharWidth() const
{
    return mnCharWidth;
}

TDVecPolyCurve* TDVecGlyph::InsertPolyCurve(int i, TDVecPolyCurve* pObject) { return insertOwned(polyCurves_, i, pObject); }
TDVecPolyCurve* TDVecGlyph::AppendPolyCurve(TDVecPolyCurve* pObject) { return appendOwned(polyCurves_, pObject); }
bool TDVecGlyph::RemovePolyCurve(int iObject) { return removeOwned(polyCurves_, iObject); }
TDVecPolyCurve* TDVecGlyph::DropPolyCurve(int iObject) { return dropOwned(polyCurves_, iObject); }
bool TDVecGlyph::DeletePolyCurve(int iObject) { return deleteOwned(polyCurves_, iObject); }
bool TDVecGlyph::ClearPolyCurves() { polyCurves_.clear(); return true; }
TDVecPolyCurve* TDVecGlyph::GetPolyCurve(int iObject) const { return getOwned(polyCurves_, iObject); }
int TDVecGlyph::CountPolyCurves() const { return static_cast<int>(polyCurves_.size()); }
void TDVecGlyph::CompactPolyCurves() { compactOwned(polyCurves_); }

void TDVecGlyph::DrawGlyph(TDGraphicEngine* pGE, bool bOutLine, unsigned int nResolution) const
{
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            curve->DrawPolyCurve(pGE, bOutLine, nResolution);
        }
    }
}

void TDVecGlyph::MoveGlyph(double X, double Y)
{
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            curve->MoveBy(X, Y);
        }
    }
    moveLine(mBaseLine, X, Y);
}

void TDVecGlyph::Rotate(TDMatPoint MatPoint, double nAngle)
{
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            curve->Rotate(MatPoint, nAngle);
        }
    }
    rotateLine(mBaseLine, MatPoint, nAngle);
}

void TDVecGlyph::TransformToPoint(TDMatPoint MatPoint)
{
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            curve->TransformToPoint(MatPoint);
        }
    }
    transformLineToPoint(mBaseLine, MatPoint);
}

void TDVecGlyph::TransformToOrigin(TDMatPoint MatPoint)
{
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            curve->TransformToOrigin(MatPoint);
        }
    }
    transformLineToOrigin(mBaseLine, MatPoint);
}

void TDVecGlyph::ToScale(TDMatPoint MatPoint, double xScale, double yScale)
{
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            curve->ToScale(MatPoint, xScale, yScale);
        }
    }
    scaleLine(mBaseLine, MatPoint, xScale, yScale);
}

void TDVecGlyph::ToShearX(double nAngle, int nFaktor)
{
    if (IsSpacing() || nAngle >= 90.0 || nAngle <= -90.0) {
        return;
    }

    const double dSin = sinD(nAngle);
    const double dCos = cosD(nAngle);
    if (dSin == 0.0 || dCos == 1.0 || dCos == -1.0 || dSin == 1.0 || dSin == -1.0 || dCos == 0.0) {
        return;
    }

    const double height = GetHeight();
    if (MatBelike2Double(height, 0.0, 4)) {
        return;
    }

    const double yDistance = mBaseLine.y1;
    double shearX = std::sqrt((height * height) * (1.0 - dCos * dCos) / (dCos * dCos));
    if (nAngle < 0.0) {
        shearX *= -1.0;
    }
    shearX *= static_cast<double>(nFaktor);

    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (!curve) {
            continue;
        }
        for (int j = 0; j < curve->CountPoints(); ++j) {
            TDMatConturPoint* point = curve->GetPointFree(j);
            if (!point) {
                continue;
            }
            point->y -= yDistance;
            point->x -= shearX * (point->y / height);
            point->y += yDistance;
        }
    }
}

void TDVecGlyph::SetColor(TDRgbColor Color)
{
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            curve->SetColor(Color);
        }
    }
}

TDMatRect TDVecGlyph::GetFrame() const
{
    TDMatRect frame = emptyRect();
    bool initialized = false;
    for (int i = 0; i < CountPolyCurves(); ++i) {
        TDVecPolyCurve* curve = GetPolyCurve(i);
        if (curve) {
            expandFrame(frame, curve->GetFrame(), initialized);
        }
    }
    return frame;
}

TDMatPoint TDVecGlyph::GetMidpoint() const
{
    const TDMatRect frame = GetFrame();
    return {(frame.P1.x + frame.P3.x) / 2.0, (frame.P1.y + frame.P3.y) / 2.0};
}

double TDVecGlyph::GetWidth() const
{
    const TDMatRect frame = GetFrame();
    return frame.P3.x - frame.P1.x;
}

double TDVecGlyph::GetHeight() const
{
    const TDMatRect frame = GetFrame();
    return frame.P3.y - frame.P1.y;
}

TDVecCharacter::TDVecCharacter()
    : mpVecGlyph(std::make_unique<TDVecGlyph>()),
      miUnicodeValue(0)
{
}

TDVecCharacter::TDVecCharacter(const TDVecCharacter& character)
    : mpVecGlyph(character.mpVecGlyph ? std::make_unique<TDVecGlyph>(*character.mpVecGlyph) : nullptr),
      miUnicodeValue(character.miUnicodeValue)
{
}

TDVecCharacter::~TDVecCharacter()
{
}

TDVecCharacter& TDVecCharacter::operator=(const TDVecCharacter& character)
{
    if (this == &character) {
        return *this;
    }
    mpVecGlyph = character.mpVecGlyph ? std::make_unique<TDVecGlyph>(*character.mpVecGlyph) : nullptr;
    miUnicodeValue = character.miUnicodeValue;
    return *this;
}

TDVecCharacter* TDVecCharacter::Clone() const { return new TDVecCharacter(*this); }

std::uint32_t TDVecCharacter::StreamFourCC()
{
    return VEDMakeFourCC('v', 'c', 'h', 'r');
}

std::unique_ptr<TDVecCharacter> TDVecCharacter::ReadFrom(VEDBinaryReader& reader)
{
    auto character = std::make_unique<TDVecCharacter>();
    character->miUnicodeValue = reader.ReadUInt16();
    const std::uint32_t signature = reader.ReadFourCC();
    if(signature != TDVecGlyph::StreamFourCC()) {
        reader.Fail(VEDBinaryError::InvalidSignature);
        return nullptr;
    }
    character->mpVecGlyph = TDVecGlyph::ReadFrom(reader);
    return reader.Ok() ? std::move(character) : nullptr;
}

void TDVecCharacter::WriteTo(VEDBinaryWriter& writer) const
{
    writer.WriteUInt16(miUnicodeValue);
    writer.WriteFourCC(TDVecGlyph::StreamFourCC());
    mpVecGlyph->WriteTo(writer);
}

void TDVecCharacter::SetUnicodeValue(unsigned short iUnicodeValue) { miUnicodeValue = iUnicodeValue; }
unsigned short TDVecCharacter::GetUnicodeValue() const { return miUnicodeValue; }
const TDVecGlyph* TDVecCharacter::GetVecGlyph() const { return mpVecGlyph.get(); }
bool TDVecCharacter::IsSpacing() const { return !mpVecGlyph || mpVecGlyph->IsSpacing(); }
void TDVecCharacter::SetBaseLine(double nAscent) { if (mpVecGlyph) mpVecGlyph->SetBaseLine(nAscent); }
TDMatLine TDVecCharacter::GetBaseLine() const { return mpVecGlyph ? mpVecGlyph->GetBaseLine() : TDMatLine{}; }
void TDVecCharacter::SetCharWidth(double nCharWidth) { if (mpVecGlyph) mpVecGlyph->SetCharWidth(nCharWidth); }
double TDVecCharacter::GetCharWidth() const { return mpVecGlyph ? mpVecGlyph->GetCharWidth() : 0.0; }
TDVecPolyCurve* TDVecCharacter::InsertPolyCurve(int i, TDVecPolyCurve* pObject) { return mpVecGlyph ? mpVecGlyph->InsertPolyCurve(i, pObject) : nullptr; }
TDVecPolyCurve* TDVecCharacter::AppendPolyCurve(TDVecPolyCurve* pObject) { return mpVecGlyph ? mpVecGlyph->AppendPolyCurve(pObject) : nullptr; }
bool TDVecCharacter::RemovePolyCurve(int iObject) { return mpVecGlyph && mpVecGlyph->RemovePolyCurve(iObject); }
TDVecPolyCurve* TDVecCharacter::DropPolyCurve(int iObject) { return mpVecGlyph ? mpVecGlyph->DropPolyCurve(iObject) : nullptr; }
bool TDVecCharacter::DeletePolyCurve(int iObject) { return mpVecGlyph && mpVecGlyph->DeletePolyCurve(iObject); }
bool TDVecCharacter::ClearPolyCurves() { return mpVecGlyph && mpVecGlyph->ClearPolyCurves(); }
TDVecPolyCurve* TDVecCharacter::GetPolyCurve(int iObject) const { return mpVecGlyph ? mpVecGlyph->GetPolyCurve(iObject) : nullptr; }
int TDVecCharacter::CountPolyCurves() const { return mpVecGlyph ? mpVecGlyph->CountPolyCurves() : 0; }
void TDVecCharacter::CompactPolyCurves() { if (mpVecGlyph) mpVecGlyph->CompactPolyCurves(); }

TDVecFont::TDVecFont()
    : mnHeight(0.0),
      mnAscent(0.0),
      mnSpacingGlyphWidth(0.0),
      msFontName{}
{
}

TDVecFont::TDVecFont(const TDVecFont& font)
    : mnHeight(font.mnHeight),
      mnAscent(font.mnAscent),
      mnSpacingGlyphWidth(font.mnSpacingGlyphWidth),
      msFontName{}
{
    characters_.reserve(font.characters_.size());
    for (const std::unique_ptr<TDVecCharacter>& character : font.characters_) {
        characters_.push_back(character ? std::make_unique<TDVecCharacter>(*character) : nullptr);
    }
    copyFixedString(msFontName, sizeof(msFontName), font.msFontName);
}

TDVecFont::~TDVecFont()
{
}

TDVecFont& TDVecFont::operator=(const TDVecFont& font)
{
    if (this == &font) {
        return *this;
    }
    characters_.clear();
    characters_.reserve(font.characters_.size());
    for (const std::unique_ptr<TDVecCharacter>& character : font.characters_) {
        characters_.push_back(character ? std::make_unique<TDVecCharacter>(*character) : nullptr);
    }
    mnHeight = font.mnHeight;
    mnAscent = font.mnAscent;
    mnSpacingGlyphWidth = font.mnSpacingGlyphWidth;
    copyFixedString(msFontName, sizeof(msFontName), font.msFontName);
    return *this;
}

TDVecFont* TDVecFont::Clone() const { return new TDVecFont(*this); }

std::uint32_t TDVecFont::StreamFourCC()
{
    return VEDMakeFourCC('v', 'f', 'n', 't');
}

std::unique_ptr<TDVecFont> TDVecFont::ReadFrom(VEDBinaryReader& reader)
{
    auto font = std::make_unique<TDVecFont>();
    font->mnHeight = reader.ReadDouble();
    font->mnAscent = reader.ReadDouble();
    font->mnSpacingGlyphWidth = reader.ReadDouble();
    copyFixedString(font->msFontName, sizeof(font->msFontName), reader.ReadCString().c_str());
    readObjectVector(reader, font->characters_);
    return reader.Ok() ? std::move(font) : nullptr;
}

void TDVecFont::WriteTo(VEDBinaryWriter& writer) const
{
    writer.WriteDouble(mnHeight);
    writer.WriteDouble(mnAscent);
    writer.WriteDouble(mnSpacingGlyphWidth);
    writer.WriteCString(msFontName);
    writeObjectVector(writer, characters_);
}

const TDVecGlyph* TDVecFont::GetGlyphFromCharacter(unsigned short iUnicodeValue) const
{
    for (int i = 0; i < CountVecCharacters(); ++i) {
        TDVecCharacter* character = GetVecCharacter(i);
        if (character && character->GetUnicodeValue() == iUnicodeValue) {
            return character->GetVecGlyph();
        }
    }
    return nullptr;
}

void TDVecFont::SetHeight(double nHeight) { mnHeight = nHeight; }
double TDVecFont::GetHeight() const { return mnHeight; }
void TDVecFont::SetAscent(double nAscent) { mnAscent = nAscent; }
double TDVecFont::GetAscent() const { return mnAscent; }
void TDVecFont::SetSpacingGlyphWidth(double nSpacingGlyphWidth) { mnSpacingGlyphWidth = nSpacingGlyphWidth; }
double TDVecFont::GetSpacingGlyphWidth() const { return mnSpacingGlyphWidth; }
void TDVecFont::SetFontName(const char* sFontName) { copyFixedString(msFontName, sizeof(msFontName), sFontName); }
const char* TDVecFont::GetFontName() const { return msFontName; }
TDVecCharacter* TDVecFont::InsertVecCharacter(int i, TDVecCharacter* pObject) { return insertOwned(characters_, i, pObject); }
TDVecCharacter* TDVecFont::AppendVecCharacter(TDVecCharacter* pObject) { return appendOwned(characters_, pObject); }
bool TDVecFont::RemoveVecCharacter(int iObject) { return removeOwned(characters_, iObject); }
TDVecCharacter* TDVecFont::DropVecCharacter(int iObject) { return dropOwned(characters_, iObject); }
bool TDVecFont::DeleteVecCharacter(int iObject) { return deleteOwned(characters_, iObject); }
bool TDVecFont::ClearVecCharacters() { characters_.clear(); return true; }
TDVecCharacter* TDVecFont::GetVecCharacter(int iObject) const { return getOwned(characters_, iObject); }
int TDVecFont::CountVecCharacters() const { return static_cast<int>(characters_.size()); }
void TDVecFont::CompactVecCharacters() { compactOwned(characters_); }

std::unique_ptr<TDVecFont> LoadVecFontFromMemory(const void* data, long size, long headerSize)
{
    if (!data || size <= headerSize || headerSize < 0) {
        return nullptr;
    }

    const char* bytes = static_cast<const char*>(data);
    VEDBinaryReader reader(bytes + headerSize, static_cast<std::size_t>(size - headerSize));
    const std::uint32_t signature = reader.ReadFourCC();
    if(signature != TDVecFont::StreamFourCC()) {
        reader.Fail(VEDBinaryError::InvalidSignature);
        return nullptr;
    }
    return TDVecFont::ReadFrom(reader);
}
