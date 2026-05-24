#include "vec_text.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <utility>

namespace {

constexpr unsigned int MAX_TEXT_RESOLUTION = 8;
constexpr unsigned int MID_TEXT_RESOLUTION = 4;

const TDVecFont* gDefaultVecTextFont = nullptr;
TDVecTextShaper gVecTextShaper;

void copyFixedString(char* destination, size_t destinationSize, const char* source)
{
    if (!destination || destinationSize == 0) {
        return;
    }
    std::snprintf(destination, destinationSize, "%s", source ? source : "");
}

TDMatRect emptyRect()
{
    return {};
}

TDMatRect rectFromBounds(double left, double top, double right, double bottom)
{
    TDMatRect rect;
    rect.P1 = {left, top};
    rect.P2 = {right, top};
    rect.P3 = {right, bottom};
    rect.P4 = {left, bottom};
    return rect;
}

void expandFrame(TDMatRect& frame, const TDMatRect& next, bool& initialized)
{
    const double left = std::min({next.P1.x, next.P2.x, next.P3.x, next.P4.x});
    const double right = std::max({next.P1.x, next.P2.x, next.P3.x, next.P4.x});
    const double top = std::min({next.P1.y, next.P2.y, next.P3.y, next.P4.y});
    const double bottom = std::max({next.P1.y, next.P2.y, next.P3.y, next.P4.y});
    if (!initialized) {
        frame = rectFromBounds(left, top, right, bottom);
        initialized = true;
        return;
    }

    const double currentLeft = std::min({frame.P1.x, frame.P2.x, frame.P3.x, frame.P4.x});
    const double currentRight = std::max({frame.P1.x, frame.P2.x, frame.P3.x, frame.P4.x});
    const double currentTop = std::min({frame.P1.y, frame.P2.y, frame.P3.y, frame.P4.y});
    const double currentBottom = std::max({frame.P1.y, frame.P2.y, frame.P3.y, frame.P4.y});
    frame = rectFromBounds(std::min(currentLeft, left), std::min(currentTop, top),
                           std::max(currentRight, right), std::max(currentBottom, bottom));
}

TDMatPoint midpoint(const TDMatRect& rect)
{
    return {(rect.P1.x + rect.P3.x) / 2.0, (rect.P1.y + rect.P3.y) / 2.0};
}

void moveRect(TDMatRect& rect, double dx, double dy)
{
    rect.P1.x += dx; rect.P1.y += dy;
    rect.P2.x += dx; rect.P2.y += dy;
    rect.P3.x += dx; rect.P3.y += dy;
    rect.P4.x += dx; rect.P4.y += dy;
}

void scalePoint(TDMatPoint& point, TDMatPoint origin, double xScale, double yScale)
{
    point.x = (point.x * xScale) + (origin.x * (1.0 - xScale));
    point.y = (point.y * yScale) + (origin.y * (1.0 - yScale));
}

void scaleRect(TDMatRect& rect, TDMatPoint origin, double xScale, double yScale)
{
    scalePoint(rect.P1, origin, xScale, yScale);
    scalePoint(rect.P2, origin, xScale, yScale);
    scalePoint(rect.P3, origin, xScale, yScale);
    scalePoint(rect.P4, origin, xScale, yScale);
}

void transformRectToPoint(TDMatRect& rect, TDMatPoint point)
{
    moveRect(rect, -point.x, -point.y);
}

void transformRectToOrigin(TDMatRect& rect, TDMatPoint point)
{
    moveRect(rect, point.x, point.y);
}

long pointOnRectNode(TDGraphicEngine* pGE, const TDMatRect& rect, double X, double Y, long tolerance)
{
    if (!pGE) {
        return -1;
    }

    const TDMatRect middle = frameMidEdges(rect);
    const POINT mouse{pGE->RealToXPos(X), pGE->RealToYPos(Y)};
    const long nodeHeight = pGE->GetNodeSize();
    for (long node : {0L, 2L, 4L, 6L, 1L, 3L, 5L, 7L}) {
        const TDMatPoint nodePoint = frameNode(rect, middle, node);
        const POINT screenNode{pGE->RealToXPos(nodePoint.x), pGE->RealToYPos(nodePoint.y)};
        if (mouseOnNode(screenNode, nodeHeight, mouse, tolerance)) {
            return node;
        }
    }
    return -1;
}

long pointOnRectNode(const TDMatRect& rect, TDMatPoint point, double tolerance)
{
    const TDMatRect middle = frameMidEdges(rect);
    for (long node : {0L, 2L, 4L, 6L, 1L, 3L, 5L, 7L}) {
        if (realPointOnNode(frameNode(rect, middle, node), point, tolerance)) {
            return node;
        }
    }
    return -1;
}

bool scaleFromMovedFrameNode(TDVecText& text, const TDMatRect& rect, long iNode, double X, double Y)
{
    if (text.GetLockResize()) {
        return false;
    }

    const TDMatRect middle = frameMidEdges(rect);
    TDMatLine p1p2Kante;
    p1p2Kante.Create(rect.P1, rect.P2);
    const double nAngle = lineAngle(p1p2Kante);
    const double dSin = sinD(nAngle);
    const double dCos = cosD(nAngle);

    double xScale = 1.0;
    double yScale = 1.0;
    TDMatPoint scaleOrigin{};

    switch (iNode) {
    case 0: {
        const TDMatPoint node0 = rect.P1;
        const TDMatPoint node4 = rect.P3;
        const TDMatPoint newNode0{node0.x + X, node0.y + Y};
        xScale = (node4.x - node0.x) != 0.0 ? ((node4.x - newNode0.x) / (node4.x - node0.x)) : 1.0;
        yScale = (node4.y - node0.y) != 0.0 ? ((node4.y - newNode0.y) / (node4.y - node0.y)) : 1.0;
        scaleOrigin = node4;
        break;
    }
    case 2: {
        const TDMatPoint node2 = rect.P2;
        const TDMatPoint node6 = rect.P4;
        const TDMatPoint newNode2{node2.x + X, node2.y + Y};
        xScale = (node6.x - node2.x) != 0.0 ? ((node6.x - newNode2.x) / (node6.x - node2.x)) : 1.0;
        yScale = (node6.y - node2.y) != 0.0 ? ((node6.y - newNode2.y) / (node6.y - node2.y)) : 1.0;
        scaleOrigin = node6;
        break;
    }
    case 4: {
        const TDMatPoint node4 = rect.P3;
        const TDMatPoint node0 = rect.P1;
        const TDMatPoint newNode4{node4.x + X, node4.y + Y};
        xScale = (node0.x - node4.x) != 0.0 ? ((node0.x - newNode4.x) / (node0.x - node4.x)) : 1.0;
        yScale = (node0.y - node4.y) != 0.0 ? ((node0.y - newNode4.y) / (node0.y - node4.y)) : 1.0;
        scaleOrigin = node0;
        break;
    }
    case 6: {
        const TDMatPoint node6 = rect.P4;
        const TDMatPoint node2 = rect.P2;
        const TDMatPoint newNode6{node6.x + X, node6.y + Y};
        xScale = (node2.x - node6.x) != 0.0 ? ((node2.x - newNode6.x) / (node2.x - node6.x)) : 1.0;
        yScale = (node2.y - node6.y) != 0.0 ? ((node2.y - newNode6.y) / (node2.y - node6.y)) : 1.0;
        scaleOrigin = node2;
        break;
    }
    case 1: {
        const TDMatPoint node1 = middle.P1;
        const TDMatPoint node5 = middle.P3;
        const TDMatPoint newNode1{node1.x + X, node1.y + Y};
        xScale = (node5.x - node1.x) != 0.0 ? ((node5.x - newNode1.x) / (node5.x - node1.x)) : 1.0;
        yScale = (node5.y - node1.y) != 0.0 ? ((node5.y - newNode1.y) / (node5.y - node1.y)) : 1.0;
        if (dSin == 0.0 || dCos == 1.0 || dCos == -1.0) {
            xScale = 1.0;
        } else {
            yScale = 1.0;
        }
        scaleOrigin = node5;
        break;
    }
    case 3: {
        const TDMatPoint node3 = middle.P2;
        const TDMatPoint node7 = middle.P4;
        const TDMatPoint newNode3{node3.x + X, node3.y + Y};
        xScale = (node7.x - node3.x) != 0.0 ? ((node7.x - newNode3.x) / (node7.x - node3.x)) : 1.0;
        yScale = (node7.y - node3.y) != 0.0 ? ((node7.y - newNode3.y) / (node7.y - node3.y)) : 1.0;
        if (dSin == 0.0 || dCos == 1.0 || dCos == -1.0) {
            yScale = 1.0;
        } else {
            xScale = 1.0;
        }
        scaleOrigin = node7;
        break;
    }
    case 5: {
        const TDMatPoint node5 = middle.P3;
        const TDMatPoint node1 = middle.P1;
        const TDMatPoint newNode5{node5.x + X, node5.y + Y};
        xScale = (node1.x - node5.x) != 0.0 ? ((node1.x - newNode5.x) / (node1.x - node5.x)) : 1.0;
        yScale = (node1.y - node5.y) != 0.0 ? ((node1.y - newNode5.y) / (node1.y - node5.y)) : 1.0;
        if (dSin == 0.0 || dCos == 1.0 || dCos == -1.0) {
            xScale = 1.0;
        } else {
            yScale = 1.0;
        }
        scaleOrigin = node1;
        break;
    }
    case 7: {
        const TDMatPoint node7 = middle.P4;
        const TDMatPoint node3 = middle.P2;
        const TDMatPoint newNode7{node7.x + X, node7.y + Y};
        xScale = (node3.x - node7.x) != 0.0 ? ((node3.x - newNode7.x) / (node3.x - node7.x)) : 1.0;
        yScale = (node3.y - node7.y) != 0.0 ? ((node3.y - newNode7.y) / (node3.y - node7.y)) : 1.0;
        if (dSin == 0.0 || dCos == 1.0 || dCos == -1.0) {
            yScale = 1.0;
        } else {
            xScale = 1.0;
        }
        scaleOrigin = node3;
        break;
    }
    default:
        return false;
    }

    return text.ToScale(scaleOrigin, xScale, yScale);
}

bool frameContainsPoint(const TDMatRect& rect, TDMatPoint point, double tolerance)
{
    const double left = std::min({rect.P1.x, rect.P2.x, rect.P3.x, rect.P4.x}) - tolerance;
    const double right = std::max({rect.P1.x, rect.P2.x, rect.P3.x, rect.P4.x}) + tolerance;
    const double top = std::min({rect.P1.y, rect.P2.y, rect.P3.y, rect.P4.y}) - tolerance;
    const double bottom = std::max({rect.P1.y, rect.P2.y, rect.P3.y, rect.P4.y}) + tolerance;
    return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
}

void appendMarkerLine(TDVecGlyph& glyph, TDMatConturPoint from, TDMatConturPoint to)
{
    auto* curve = new TDVecPolyCurve();
    curve->SetResolution(MID_TEXT_RESOLUTION);
    curve->SetShowControls(false);
    curve->SetShowPolygon(false);
    from.eType = CPT_PRIME_LINE;
    from.bJoint = true;
    to.eType = CPT_PRIME_LINE;
    to.bJoint = true;
    curve->AppendPoint(from);
    curve->AppendPoint(to);
    glyph.AppendPolyCurve(curve);
}

std::unique_ptr<TDVecGlyph> makeMissingGlyphMarker(const TDVecFont& font)
{
    for (const unsigned short marker : {'?', '!', 'X'}) {
        if (const TDVecGlyph* glyph = font.GetGlyphFromCharacter(marker)) {
            return std::unique_ptr<TDVecGlyph>(glyph->Clone());
        }
    }

    const double height = font.GetHeight() > 0.0 ? font.GetHeight() : 1000.0;
    const double width = font.GetSpacingGlyphWidth() > 0.0 ? font.GetSpacingGlyphWidth() : height * 0.6;
    const double inset = std::min(width, height) * 0.12;
    const double left = inset;
    const double right = width - inset;
    const double top = inset;
    const double bottom = height - inset;

    auto glyph = std::make_unique<TDVecGlyph>();
    glyph->SetCharWidth(width);
    appendMarkerLine(*glyph, {left, top}, {right, bottom});
    appendMarkerLine(*glyph, {right, top}, {left, bottom});
    glyph->SetBaseLine(font.GetAscent());
    return glyph;
}

std::unique_ptr<TDVecGlyph> cloneGlyphOrMissingMarker(const TDVecFont& font, unsigned int codePoint)
{
    if (codePoint <= 0xFFFFU) {
        if (const TDVecGlyph* glyph = font.GetGlyphFromCharacter(static_cast<unsigned short>(codePoint))) {
            return std::unique_ptr<TDVecGlyph>(glyph->Clone());
        }
    }
    return makeMissingGlyphMarker(font);
}

unsigned int decodeNextUtf8OrLatin1(const char*& text)
{
    // New text is UTF-8, while old VED documents may still contain single-byte
    // text. Invalid UTF-8 bytes intentionally fall back to their Latin-1 value.
    const auto* bytes = reinterpret_cast<const unsigned char*>(text);
    if (*bytes == '\0') {
        return 0;
    }
    if (*bytes < 0x80) {
        ++text;
        return *bytes;
    }
    if ((bytes[0] & 0xE0U) == 0xC0U && (bytes[1] & 0xC0U) == 0x80U) {
        const unsigned int value = ((bytes[0] & 0x1FU) << 6U) | (bytes[1] & 0x3FU);
        text += 2;
        return value;
    }
    if ((bytes[0] & 0xF0U) == 0xE0U && (bytes[1] & 0xC0U) == 0x80U && (bytes[2] & 0xC0U) == 0x80U) {
        const unsigned int value = ((bytes[0] & 0x0FU) << 12U) | ((bytes[1] & 0x3FU) << 6U) | (bytes[2] & 0x3FU);
        text += 3;
        return value;
    }
    if ((bytes[0] & 0xF8U) == 0xF0U && (bytes[1] & 0xC0U) == 0x80U && (bytes[2] & 0xC0U) == 0x80U && (bytes[3] & 0xC0U) == 0x80U) {
        const unsigned int value = ((bytes[0] & 0x07U) << 18U) | ((bytes[1] & 0x3FU) << 12U) | ((bytes[2] & 0x3FU) << 6U) | (bytes[3] & 0x3FU);
        text += 4;
        return value;
    }

    ++text;
    return *bytes;
}

} // namespace

TDVecText::TDVecText()
    : mOriginPoint{0.0, 0.0},
      mnXScale(1.0),
      mnYScale(1.0),
      mnAngle(0.0),
      mnIncline(0.0),
      mnLineSpacing(1.0),
      mnCharSpacing(1.0),
      mbVertical(false),
      mbUnderline(false),
      meJustification(JUST_LEFT),
      meVerticalAlignment(VALIGN_TOP),
      mnResolution(MID_TEXT_RESOLUTION),
      msFontName{},
      mpVecFont(gDefaultVecTextFont),
      msText(new char[1])
{
    msText[0] = '\0';
    SetType(VECOBJ_TEX);
    if (mpVecFont) {
        SetFontName(mpVecFont->GetFontName());
    }
}

TDVecText::TDVecText(const TDVecText& other)
    : TDVecObject(other),
      msText(nullptr)
{
    CopyFrom(other);
}

TDVecText& TDVecText::operator=(const TDVecText& other)
{
    if (this == &other) {
        return *this;
    }
    CopyFrom(other);
    return *this;
}

TDVecText::~TDVecText()
{
    delete[] msText;
}

void TDVecText::CopyFrom(const TDVecText& other)
{
    mOriginPoint = other.mOriginPoint;
    mnXScale = other.mnXScale;
    mnYScale = other.mnYScale;
    mnAngle = other.mnAngle;
    mnIncline = other.mnIncline;
    mnLineSpacing = other.mnLineSpacing;
    mnCharSpacing = other.mnCharSpacing;
    mbVertical = other.mbVertical;
    mbUnderline = other.mbUnderline;
    meJustification = other.meJustification;
    meVerticalAlignment = other.meVerticalAlignment;
    mnResolution = other.mnResolution;
    mpVecFont = other.mpVecFont;
    copyFixedString(msFontName, sizeof(msFontName), other.msFontName);
    SetText(other.msText);
}

std::uint32_t TDVecText::StreamFourCC()
{
    return VEDMakeFourCC('v', 't', 'x', 't');
}

std::uint32_t TDVecText::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecText> TDVecText::ReadFrom(VEDBinaryReader& reader)
{
    auto text = std::make_unique<TDVecText>();
    text->ReadObjectStateFrom(reader);
    text->mOriginPoint.x = reader.ReadDouble();
    text->mOriginPoint.y = reader.ReadDouble();
    text->mnXScale = reader.ReadDouble();
    text->mnYScale = reader.ReadDouble();
    text->mnAngle = reader.ReadDouble();
    text->mnIncline = reader.ReadDouble();
    text->mnLineSpacing = reader.ReadDouble();
    text->mnCharSpacing = reader.ReadDouble();
    text->mbVertical = reader.ReadBool();
    text->mbUnderline = reader.ReadBool();
    text->meJustification = static_cast<TDJustification>(reader.ReadEnum());
    text->mnResolution = reader.ReadUInt32();
    copyFixedString(text->msFontName, sizeof(text->msFontName), reader.ReadCString().c_str());
    text->SetText(reader.ReadCString().c_str());
    text->meVerticalAlignment = static_cast<TDVerticalAlignment>(reader.ReadEnum());
    text->mpVecFont = gDefaultVecTextFont;
    text->InitializeGlyphs();
    return reader.Ok() ? std::move(text) : nullptr;
}

void TDVecText::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteDouble(mOriginPoint.x);
    writer.WriteDouble(mOriginPoint.y);
    writer.WriteDouble(mnXScale);
    writer.WriteDouble(mnYScale);
    writer.WriteDouble(mnAngle);
    writer.WriteDouble(mnIncline);
    writer.WriteDouble(mnLineSpacing);
    writer.WriteDouble(mnCharSpacing);
    writer.WriteBool(mbVertical);
    writer.WriteBool(mbUnderline);
    writer.WriteEnum(meJustification);
    writer.WriteUInt32(mnResolution);
    writer.WriteCString(msFontName);
    writer.WriteCString(msText ? msText : "");
    writer.WriteEnum(meVerticalAlignment);
}

void TDVecText::Draw(TDGraphicEngine* pGE, bool bOutLine) const
{
    if (!pGE) {
        return;
    }
    EnsureGlyphsInitialized();
    TDVecObject::Draw(pGE, bOutLine);
    for (int i = 0; i < CountGlyphs(); ++i) {
        TDVecGlyph* glyph = glyphs_[static_cast<size_t>(i)].get();
        if (glyph) {
            glyph->SetColor(GetColor());
            glyph->DrawGlyph(pGE, bOutLine, mnResolution);
        }
    }
}

void TDVecText::DrawNodes(TDGraphicEngine* pGE) const
{
    if (!pGE) {
        return;
    }
    DrawFrame(pGE);
    pGE->DrawNode(mOriginPoint.x, mOriginPoint.y, NODE_CONTROL, false);
}

TDMatRect TDVecText::GetFrame() const
{
    EnsureGlyphsInitialized();
    TDMatRect frame = emptyRect();
    bool initialized = false;
    for (int i = 0; i < CountGlyphs(); ++i) {
        TDVecGlyph* glyph = glyphs_[static_cast<size_t>(i)].get();
        if (glyph && !glyph->IsSpacing()) {
            expandFrame(frame, glyph->GetFrame(), initialized);
        }
    }
    return frame;
}

TDMatPoint TDVecText::GetMidpoint() const
{
    return midpoint(GetFrame());
}

std::unique_ptr<TDVecObject> TDVecText::Clone() const
{
    return std::make_unique<TDVecText>(*this);
}

TDVecLineHitResult TDVecText::HitTest(TDMatPoint point, double tolerance) const
{
    return frameContainsPoint(GetFrame(), point, tolerance) ? TDVecLineHitResult{TDVecLineHitKind::Body, 0.0} : TDVecLineHitResult{};
}

TDVecLineHitResult TDVecText::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const
{
    (void)pGE;
    return HitTest(point, static_cast<double>(tolerancePixels));
}

long TDVecText::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const
{
    return PointOnFrameNode(pGE, X, Y, Toleranz);
}

long TDVecText::PointOnNode(TDMatPoint point, double tolerance) const
{
    return PointOnFrameNode(point, tolerance);
}

bool TDVecText::MoveBy(double dx, double dy)
{
    if (GetLockPosition()) {
        return false;
    }
    InitializeOriginPoint({mOriginPoint.x + dx, mOriginPoint.y + dy});
    return true;
}

bool TDVecText::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint)
{
    return MoveFrameNode(iNode, X, Y, MatCPoint);
}

bool TDVecText::ToScale(TDMatPoint MatPoint, double xScale, double yScale)
{
    if (GetLockResize() || MatBelike2Double(xScale, 0.0, 4) || MatBelike2Double(yScale, 0.0, 4)) {
        return false;
    }
    mOriginPoint.x = (mOriginPoint.x * xScale) + (MatPoint.x * (1.0 - xScale));
    mOriginPoint.y = (mOriginPoint.y * yScale) + (MatPoint.y * (1.0 - yScale));
    if (sinD(mnAngle) == 0.0 || cosD(mnAngle) == 1.0 || cosD(mnAngle) == -1.0) {
        mnXScale *= xScale;
        mnYScale *= yScale;
    } else {
        mnXScale *= yScale;
        mnYScale *= xScale;
    }
    MarkGlyphsDirty();
    return true;
}

bool TDVecText::Rotate(TDMatPoint MatPoint, double nAngle)
{
    if (GetLockPosition()) {
        return false;
    }
    const TDMatPoint translated{mOriginPoint.x - MatPoint.x, mOriginPoint.y - MatPoint.y};
    mOriginPoint.x = cosD(nAngle) * translated.x - sinD(nAngle) * translated.y + MatPoint.x;
    mOriginPoint.y = sinD(nAngle) * translated.x + cosD(nAngle) * translated.y + MatPoint.y;
    mnAngle += nAngle;
    MarkGlyphsDirty();
    return true;
}

void TDVecText::TransformToPoint(TDMatPoint MatPoint)
{
    InitializeOriginPoint({mOriginPoint.x - MatPoint.x, mOriginPoint.y - MatPoint.y});
}

void TDVecText::TransformToOrigin(TDMatPoint MatPoint)
{
    InitializeOriginPoint({mOriginPoint.x + MatPoint.x, mOriginPoint.y + MatPoint.y});
}

void TDVecText::Initialize()
{
    while (mnAngle >= 360.0) {
        mnAngle -= 360.0;
    }
    while (mnAngle <= -360.0) {
        mnAngle += 360.0;
    }
    MarkGlyphsDirty();
}

void TDVecText::EnsureGlyphsInitialized() const
{
    if (!glyphsDirty_) {
        return;
    }
    const_cast<TDVecText*>(this)->InitializeGlyphs();
}

void TDVecText::MarkGlyphsDirty()
{
    glyphsDirty_ = true;
}

void TDVecText::InitializeGlyphs()
{
    glyphsDirty_ = false;
    glyphs_.clear();
    if (!msText || !mpVecFont) {
        return;
    }

    std::vector<TDVecShapedGlyph> shapedGlyphs;
    if (gVecTextShaper && gVecTextShaper(mpVecFont, msText, shapedGlyphs)) {
        // Complex scripts and TT fonts can arrive as final glyph outlines plus
        // advances from the app-layer shaper. The legacy byte path below remains
        // for built-in VFN fonts and old documents.
        double charsX = 0.0;
        double charsY = 0.0;
        for (TDVecShapedGlyph& shapedGlyph : shapedGlyphs) {
            if (shapedGlyph.lineBreak) {
                charsY += mpVecFont->GetHeight() + mnLineSpacing;
                charsX = 0.0;
                continue;
            }
            std::unique_ptr<TDVecGlyph> glyphOwner = std::move(shapedGlyph.glyph);
            if (!glyphOwner) {
                glyphOwner = makeMissingGlyphMarker(*mpVecFont);
            }
            TDVecGlyph* glyph = glyphOwner.release();
            glyph->MoveGlyph(mOriginPoint.x + charsX + shapedGlyph.xOffset, mOriginPoint.y + charsY + shapedGlyph.yOffset);
            glyph->ToShearX(mnIncline, 1);
            glyph->ToScale(mOriginPoint, mnXScale, mnYScale);
            glyph->Rotate(mOriginPoint, mnAngle);
            glyphs_.push_back(std::unique_ptr<TDVecGlyph>(glyph));
            charsX += shapedGlyph.xAdvance + mnCharSpacing;
            charsY += shapedGlyph.yAdvance;
        }
        return;
    }

    double charsX = 0.0;
    double charsY = 0.0;
    const char* text = msText;
    while (text && *text) {
        const unsigned int codePoint = decodeNextUtf8OrLatin1(text);
        if (codePoint == '\n') {
            charsY += mpVecFont->GetHeight() + mnLineSpacing;
            charsX = 0.0;
            continue;
        }
        std::unique_ptr<TDVecGlyph> glyphOwner = cloneGlyphOrMissingMarker(*mpVecFont, codePoint);
        if (!glyphOwner) {
            continue;
        }
        TDVecGlyph* glyph = glyphOwner.release();
        const double xGlyph = mOriginPoint.x + charsX;
        const double yGlyph = mOriginPoint.y + charsY;
        charsX += (glyph->IsSpacing() ? mpVecFont->GetSpacingGlyphWidth() : glyph->GetCharWidth()) + mnCharSpacing;
        glyph->MoveGlyph(xGlyph, yGlyph);
        glyph->ToShearX(mnIncline, 1);
        glyph->ToScale(mOriginPoint, mnXScale, mnYScale);
        glyph->Rotate(mOriginPoint, mnAngle);
        glyphs_.push_back(std::unique_ptr<TDVecGlyph>(glyph));
    }
}

void TDVecText::SetText(const char* sText)
{
    delete[] msText;
    if (!sText || IsStringBlank(sText)) {
        msText = new char[1];
        msText[0] = '\0';
    } else {
        const size_t size = std::strlen(sText) + 1;
        msText = new char[size];
        std::memcpy(msText, sText, size);
    }
    MarkGlyphsDirty();
}

const char* TDVecText::GetText() const { return msText ? msText : ""; }

void TDVecText::SetOriginPoint(TDMatPoint OriginPoint) { InitializeOriginPoint(OriginPoint); }
TDMatPoint TDVecText::GetOriginPoint() const { return mOriginPoint; }
void TDVecText::InitializeOriginPoint(TDMatPoint OriginPoint)
{
    const double dx = OriginPoint.x - mOriginPoint.x;
    const double dy = OriginPoint.y - mOriginPoint.y;
    for (int i = 0; i < CountGlyphs(); ++i) {
        TDVecGlyph* glyph = glyphs_[static_cast<size_t>(i)].get();
        if (glyph) {
            glyph->MoveGlyph(dx, dy);
        }
    }
    mOriginPoint = OriginPoint;
}

void TDVecText::SetXScale(double nXScale) { if (!MatBelike2Double(nXScale, 0.0, 4)) { mnXScale = nXScale; MarkGlyphsDirty(); } }
double TDVecText::GetXScale() const { return mnXScale; }
void TDVecText::SetYScale(double nYScale) { if (!MatBelike2Double(nYScale, 0.0, 4)) { mnYScale = nYScale; MarkGlyphsDirty(); } }
double TDVecText::GetYScale() const { return mnYScale; }
void TDVecText::SetAngle(double nAngle) { mnAngle = nAngle; MarkGlyphsDirty(); }
double TDVecText::GetAngle() const { return mnAngle; }
void TDVecText::SetIncline(double nIncline) { mnIncline = nIncline; MarkGlyphsDirty(); }
double TDVecText::GetIncline() const { return mnIncline; }
void TDVecText::SetLineSpacing(double nLineSpacing) { mnLineSpacing = nLineSpacing; MarkGlyphsDirty(); }
double TDVecText::GetLineSpacing() const { return mnLineSpacing; }
void TDVecText::SetCharSpacing(double nCharSpacing) { mnCharSpacing = nCharSpacing; MarkGlyphsDirty(); }
double TDVecText::GetCharSpacing() const { return mnCharSpacing; }
void TDVecText::SetVertical(bool bVertical) { mbVertical = bVertical; }
bool TDVecText::GetVertical() const { return mbVertical; }
void TDVecText::SetUnderline(bool bUnderline) { mbUnderline = bUnderline; }
bool TDVecText::GetUnderline() const { return mbUnderline; }
void TDVecText::SetJustification(TDJustification eJustification) { meJustification = eJustification; MarkGlyphsDirty(); }
TDVecText::TDJustification TDVecText::GetJustification() const { return meJustification; }
void TDVecText::SetVerticalAlignment(TDVerticalAlignment eVerticalAlignment) { meVerticalAlignment = eVerticalAlignment; MarkGlyphsDirty(); }
TDVecText::TDVerticalAlignment TDVecText::GetVerticalAlignment() const { return meVerticalAlignment; }
void TDVecText::SetFontName(const char* sFontName) { copyFixedString(msFontName, sizeof(msFontName), sFontName); }
const char* TDVecText::GetFontName() const { return msFontName; }
void TDVecText::SetVecFontPointer(const TDVecFont* pVecFont) { mpVecFont = pVecFont; MarkGlyphsDirty(); }
const TDVecFont* TDVecText::GetVecFontPointer() const { return mpVecFont; }
void TDVecText::SetResolution(unsigned int nResolution) { mnResolution = nResolution <= MAX_TEXT_RESOLUTION ? nResolution : MID_TEXT_RESOLUTION; }
unsigned int TDVecText::GetResolution() const { return mnResolution; }
void TDVecText::SetScaleDependency(bool bValue) { (void)bValue; }
bool TDVecText::GetScaleDependency() const { return true; }
int TDVecText::CountGlyphs() const { EnsureGlyphsInitialized(); return static_cast<int>(glyphs_.size()); }

void TDVecText::SetParameter(const TDVecTextParameter* p)
{
    if (!p || MatBelike2Double(p->mnXScale, 0.0, 4) || MatBelike2Double(p->mnYScale, 0.0, 4)) {
        return;
    }
    mnXScale = p->mnXScale;
    mnYScale = p->mnYScale;
    mnAngle = p->mnAngle;
    mnIncline = p->mnIncline;
    mnLineSpacing = p->mnLineSpacing;
    mnCharSpacing = p->mnCharSpacing;
    mbVertical = p->mbVertical;
    mbUnderline = p->mbUnderline;
    meJustification = p->meJustification;
    meVerticalAlignment = p->meVerticalAlignment;
    SetResolution(p->mnResolution);
    mpVecFont = p->mpVecFont ? p->mpVecFont : gDefaultVecTextFont;
    SetFontName(p->msFontName);
    MarkGlyphsDirty();
}

void TDVecText::GetParameter(TDVecTextParameter* p) const
{
    if (!p) {
        return;
    }
    p->mnXScale = mnXScale;
    p->mnYScale = mnYScale;
    p->mnAngle = mnAngle;
    p->mnIncline = mnIncline;
    p->mnLineSpacing = mnLineSpacing;
    p->mnCharSpacing = mnCharSpacing;
    p->mbVertical = mbVertical;
    p->mbUnderline = mbUnderline;
    p->meJustification = meJustification;
    p->meVerticalAlignment = meVerticalAlignment;
    p->mnResolution = mnResolution;
    p->mpVecFont = mpVecFont;
    p->mbScaleDependency = true;
    copyFixedString(p->msFontName, sizeof(p->msFontName), msFontName);
}

TDVecFrameText::TDVecFrameText()
    : TDVecText(),
      mRectangle{},
      mbOriginIsjustified(false),
      mbScaleDependency(true)
{
    SetType(VECOBJ_FRAMETEXT);
}

TDVecFrameText::TDVecFrameText(const TDVecFrameText& other)
    : TDVecText(other),
      mRectangle(other.mRectangle),
      mbOriginIsjustified(other.mbOriginIsjustified),
      mbScaleDependency(other.mbScaleDependency)
{
}

TDVecFrameText& TDVecFrameText::operator=(const TDVecFrameText& other)
{
    if (this == &other) {
        return *this;
    }
    TDVecText::operator=(other);
    mRectangle = other.mRectangle;
    mbOriginIsjustified = other.mbOriginIsjustified;
    mbScaleDependency = other.mbScaleDependency;
    return *this;
}

std::uint32_t TDVecFrameText::StreamFourCC()
{
    return VEDMakeFourCC('f', 't', 'x', 't');
}

std::uint32_t TDVecFrameText::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecFrameText> TDVecFrameText::ReadFrom(VEDBinaryReader& reader)
{
    auto text = std::make_unique<TDVecFrameText>();
    text->ReadObjectStateFrom(reader);
    text->mOriginPoint.x = reader.ReadDouble();
    text->mOriginPoint.y = reader.ReadDouble();
    text->mnXScale = reader.ReadDouble();
    text->mnYScale = reader.ReadDouble();
    text->mnAngle = reader.ReadDouble();
    text->mnIncline = reader.ReadDouble();
    text->mnLineSpacing = reader.ReadDouble();
    text->mnCharSpacing = reader.ReadDouble();
    text->mbVertical = reader.ReadBool();
    text->mbUnderline = reader.ReadBool();
    text->meJustification = static_cast<TDJustification>(reader.ReadEnum());
    text->mnResolution = reader.ReadUInt32();
    copyFixedString(text->msFontName, sizeof(text->msFontName), reader.ReadCString().c_str());
    text->SetText(reader.ReadCString().c_str());
    text->meVerticalAlignment = static_cast<TDVerticalAlignment>(reader.ReadEnum());
    text->mpVecFont = gDefaultVecTextFont;
    text->mRectangle.P1.x = reader.ReadDouble();
    text->mRectangle.P1.y = reader.ReadDouble();
    text->mRectangle.P2.x = reader.ReadDouble();
    text->mRectangle.P2.y = reader.ReadDouble();
    text->mRectangle.P3.x = reader.ReadDouble();
    text->mRectangle.P3.y = reader.ReadDouble();
    text->mRectangle.P4.x = reader.ReadDouble();
    text->mRectangle.P4.y = reader.ReadDouble();
    text->mbScaleDependency = reader.ReadBool();
    text->InitializeGlyphs();
    return reader.Ok() ? std::move(text) : nullptr;
}

void TDVecFrameText::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecText::WriteTo(writer);
    writer.WriteDouble(mRectangle.P1.x);
    writer.WriteDouble(mRectangle.P1.y);
    writer.WriteDouble(mRectangle.P2.x);
    writer.WriteDouble(mRectangle.P2.y);
    writer.WriteDouble(mRectangle.P3.x);
    writer.WriteDouble(mRectangle.P3.y);
    writer.WriteDouble(mRectangle.P4.x);
    writer.WriteDouble(mRectangle.P4.y);
    writer.WriteBool(mbScaleDependency);
}

void TDVecFrameText::Draw(TDGraphicEngine* pGE, bool bOutLine) const
{
    if (!pGE) {
        return;
    }
    TDVecObject::Draw(pGE, bOutLine);
    pGE->DrawRectangle(&mRectangle, bOutLine);
    TDVecText::Draw(pGE, bOutLine);
}

void TDVecFrameText::DrawNodes(TDGraphicEngine* pGE) const
{
    if (!pGE) {
        return;
    }
    DrawFrame(pGE);
    pGE->DrawNode(mOriginPoint.x, mOriginPoint.y, NODE_CONTROL, false);
}

TDMatRect TDVecFrameText::GetFrame() const
{
    TDMatRect frame = mRectangle;
    bool initialized = true;
    if (!IsStringBlank(msText)) {
        expandFrame(frame, GetTextFrame(), initialized);
    }
    return frame;
}

TDMatPoint TDVecFrameText::GetMidpoint() const
{
    return midpoint(mRectangle);
}

std::unique_ptr<TDVecObject> TDVecFrameText::Clone() const
{
    return std::make_unique<TDVecFrameText>(*this);
}

bool TDVecFrameText::MoveBy(double dx, double dy)
{
    if (!TDVecText::MoveBy(dx, dy)) {
        return false;
    }
    moveRect(mRectangle, dx, dy);
    return true;
}

long TDVecFrameText::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const
{
    if (!GetSelect()) {
        return -1;
    }
    return pointOnRectNode(pGE, mRectangle, X, Y, Toleranz);
}

long TDVecFrameText::PointOnNode(TDMatPoint point, double tolerance) const
{
    if (!GetSelect()) {
        return -1;
    }
    return pointOnRectNode(mRectangle, point, tolerance);
}

bool TDVecFrameText::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint)
{
    (void)MatCPoint;
    return scaleFromMovedFrameNode(*this, mRectangle, iNode, X, Y);
}

bool TDVecFrameText::ToScale(TDMatPoint MatPoint, double xScale, double yScale)
{
    if (GetLockResize()) {
        return false;
    }
    if (mbScaleDependency) {
        TDVecText::ToScale(MatPoint, xScale, yScale);
    }
    scaleRect(mRectangle, MatPoint, xScale, yScale);
    MarkGlyphsDirty();
    return true;
}

void TDVecFrameText::TransformToPoint(TDMatPoint MatPoint)
{
    TDVecText::TransformToPoint(MatPoint);
    transformRectToPoint(mRectangle, MatPoint);
}

void TDVecFrameText::TransformToOrigin(TDMatPoint MatPoint)
{
    TDVecText::TransformToOrigin(MatPoint);
    transformRectToOrigin(mRectangle, MatPoint);
}

void TDVecFrameText::SetRectangle(const TDMatRect* pRectangle)
{
    if (!pRectangle) {
        return;
    }
    mRectangle = *pRectangle;
    mbOriginIsjustified = false;
    MarkGlyphsDirty();
}

TDMatRect TDVecFrameText::GetRectangle() const { return mRectangle; }
TDMatRect TDVecFrameText::GetTextFrame() const { return TDVecText::GetFrame(); }
void TDVecFrameText::SetScaleDependency(bool bValue) { mbScaleDependency = bValue; }
bool TDVecFrameText::GetScaleDependency() const { return mbScaleDependency; }

void TDVecFrameText::InitializeGlyphs()
{
    TDVecText::InitializeGlyphs();
    JustifyOriginPoint();
}

void TDVecFrameText::JustifyOriginPoint()
{
    if (mbOriginIsjustified || IsStringBlank(msText)) {
        return;
    }

    TDMatPoint newOrigin = mRectangle.P1;
    const TDMatRect textFrame = GetTextFrame();
    const double frameWidth = MatDistance2Point(mRectangle.P1, mRectangle.P2);
    const double frameHeight = MatDistance2Point(mRectangle.P1, mRectangle.P4);
    const double textWidth = MatDistance2Point(textFrame.P1, textFrame.P2);
    const double textHeight = MatDistance2Point(textFrame.P1, textFrame.P4);
    const double xDiff = textFrame.P1.x - mOriginPoint.x;
    const double yDiff = textFrame.P1.y - mOriginPoint.y;

    if (meJustification == JUST_CENTER) {
        newOrigin.x = mRectangle.P1.x + ((frameWidth - textWidth - xDiff) / 2.0);
    } else if (meJustification == JUST_RIGHT) {
        newOrigin.x = mRectangle.P1.x + (frameWidth - textWidth - xDiff);
    } else {
        newOrigin.x = mRectangle.P1.x - xDiff;
    }

    if (meVerticalAlignment == VALIGN_CENTER) {
        newOrigin.y = mRectangle.P1.y + ((frameHeight - textHeight - yDiff) / 2.0);
    } else if (meVerticalAlignment == VALIGN_BOTTOM) {
        newOrigin.y = mRectangle.P1.y + (frameHeight - textHeight - yDiff);
    } else {
        newOrigin.y = mRectangle.P1.y;
    }

    InitializeOriginPoint(newOrigin);
    mbOriginIsjustified = true;
}

void TDVecFrameText::SetParameter(const TDVecTextParameter* p)
{
    TDVecText::SetParameter(p);
    if (p) {
        mbScaleDependency = p->mbScaleDependency;
    }
    mbOriginIsjustified = false;
    MarkGlyphsDirty();
}

void TDVecFrameText::GetParameter(TDVecTextParameter* p) const
{
    TDVecText::GetParameter(p);
    if (p) {
        p->mbScaleDependency = mbScaleDependency;
    }
}

bool IsStringBlank(const char* s)
{
    if (!s) {
        return true;
    }
    while (*s) {
        if (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') {
            return false;
        }
        ++s;
    }
    return true;
}

void SetDefaultVecTextFont(const TDVecFont* font)
{
    gDefaultVecTextFont = font;
}

const TDVecFont* GetDefaultVecTextFont()
{
    return gDefaultVecTextFont;
}

void SetVecTextShaper(TDVecTextShaper shaper)
{
    gVecTextShaper = std::move(shaper);
}
