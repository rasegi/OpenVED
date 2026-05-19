#include "vec_object.h"

#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <array>
#include <cmath>
#include <array>

namespace {

long pointOnFrameNode(
    const TDMatRect& frame,
    TDGraphicEngine* pGE,
    double X,
    double Y,
    long Toleranz)
{
    if (!pGE) {
        return -1;
    }

    const TDMatRect middle = frameMidEdges(frame);
    const POINT mouse{pGE->RealToXPos(X), pGE->RealToYPos(Y)};
    const long nodeHeight = pGE->GetNodeSize();
    const std::array<POINT, 8> nodes{{
        {pGE->RealToXPos(frame.P1.x), pGE->RealToYPos(frame.P1.y)},
        {pGE->RealToXPos(middle.P1.x), pGE->RealToYPos(middle.P1.y)},
        {pGE->RealToXPos(frame.P2.x), pGE->RealToYPos(frame.P2.y)},
        {pGE->RealToXPos(middle.P2.x), pGE->RealToYPos(middle.P2.y)},
        {pGE->RealToXPos(frame.P3.x), pGE->RealToYPos(frame.P3.y)},
        {pGE->RealToXPos(middle.P3.x), pGE->RealToYPos(middle.P3.y)},
        {pGE->RealToXPos(frame.P4.x), pGE->RealToYPos(frame.P4.y)},
        {pGE->RealToXPos(middle.P4.x), pGE->RealToYPos(middle.P4.y)}
    }};

    for (long node : {0L, 2L, 4L, 6L, 1L, 3L, 5L, 7L}) {
        if (mouseOnNode(nodes[static_cast<std::size_t>(node)], nodeHeight, mouse, Toleranz)) {
            return node;
        }
    }
    return -1;
}

long pointOnFrameNode(const TDMatRect& frame, TDMatPoint point, double tolerance)
{
    const TDMatRect middle = frameMidEdges(frame);
    for (long node : {0L, 2L, 4L, 6L, 1L, 3L, 5L, 7L}) {
        if (realPointOnNode(frameNode(frame, middle, node), point, tolerance)) {
            return node;
        }
    }
    return -1;
}

std::array<TDMatPoint, 8> frameNodes(const TDMatRect& frame)
{
    const TDMatRect middle = frameMidEdges(frame);
    return {{
        frame.P1,
        middle.P1,
        frame.P2,
        middle.P2,
        frame.P3,
        middle.P3,
        frame.P4,
        middle.P4
    }};
}

bool isFrameEdgeHorizontal(const TDMatRect& frame)
{
    TDMatLine p1p2Kante;
    p1p2Kante.Create(frame.P1, frame.P2);
    const double nAngle = lineAngle(p1p2Kante);
    const double dSin = sinD(nAngle);
    const double dCos = cosD(nAngle);
    return dSin == 0.0 || dCos == 1.0 || dCos == -1.0;
}

void constrainMiddleNodeScale(long node, bool horizontalFrame, double& xScale, double& yScale)
{
    if ((node % 2) == 0) {
        return;
    }

    // Middle handles resize only one axis. Which axis is fixed depends on
    // whether the frame's top edge is horizontal or rotated.
    const bool topOrBottomHandle = node == 1 || node == 5;
    if (topOrBottomHandle == horizontalFrame) {
        xScale = 1.0;
    } else {
        yScale = 1.0;
    }
}

double scaleFactor(double anchor, double original, double moved)
{
    return (anchor - original) != 0.0 ? ((anchor - moved) / (anchor - original)) : 1.0;
}

bool moveFrameNodeFromFrame(TDVecObject& object, const TDMatRect& frame, long iNode, double X, double Y)
{
    if (iNode < 0 || iNode > 7) {
        return false;
    }

    // Nodes are numbered clockwise: corners are 0/2/4/6, edge middles are
    // 1/3/5/7. Resizing anchors at the opposite node, like the old VED frame.
    const auto nodes = frameNodes(frame);
    const auto node = static_cast<std::size_t>(iNode);
    const auto oppositeNode = static_cast<std::size_t>((iNode + 4) % 8);
    const TDMatPoint original = nodes[node];
    const TDMatPoint moved{original.x + X, original.y + Y};
    const TDMatPoint anchor = nodes[oppositeNode];
    double xScale = scaleFactor(anchor.x, original.x, moved.x);
    double yScale = scaleFactor(anchor.y, original.y, moved.y);
    constrainMiddleNodeScale(iNode, isFrameEdgeHorizontal(frame), xScale, yScale);
    if (object.GetResizeProportional()) {
        const int xSign = xScale < 0.0 ? -1 : 1;
        const int ySign = yScale < 0.0 ? -1 : 1;
        if (xScale == 1.0 || yScale == 1.0) {
            if (xScale == 1.0) {
                xScale = std::fabs(yScale);
            } else {
                yScale = std::fabs(xScale);
            }
        } else if (std::fabs(xScale) < std::fabs(yScale)) {
            yScale = std::fabs(xScale) * static_cast<double>(ySign);
        } else if (std::fabs(yScale) < std::fabs(xScale)) {
            xScale = std::fabs(yScale) * static_cast<double>(xSign);
        }
    }
    return object.ToScale(anchor, xScale, yScale);
}

} // namespace

bool TDVecLineHitResult::IsHit() const {
    return kind != TDVecLineHitKind::None;
}

bool TDVecLineHitResult::IsNodeHit() const {
    return kind == TDVecLineHitKind::StartNode ||
           kind == TDVecLineHitKind::EndNode ||
           kind == TDVecLineHitKind::MidpointNode;
}

TDVecObject::TDVecObject()
    : type_(VECOBJ_UNKNOWN),
      selected_(false),
      lockResize_(false),
      resizeProportional_(false),
      lockPosition_(false),
      color_(0),
      parentModel_(nullptr) {
}

TDVecObject::TDVecObject(VEDBinaryReader& reader)
    : type_(VECOBJ_UNKNOWN),
      selected_(false),
      lockResize_(false),
      resizeProportional_(false),
      lockPosition_(false),
      color_(0),
      parentModel_(nullptr) {
    ReadObjectStateFrom(reader);
}

void TDVecObject::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    (void)bOutLine;
    if (!pGE) {
        return;
    }

    pGE->SetDrawColor(color_);
}

TDMatRect TDVecObject::GetScaleFrame() const {
    return GetFrame();
}

void TDVecObject::DrawFrame(TDGraphicEngine* pGE) const {
    if (!pGE) {
        return;
    }

    TDMatRect frame = GetScaleFrame();
    pGE->DrawFrame(&frame);

    const bool resizeLock = GetLockResize();
    const TDMatPoint midTop{(frame.P1.x + frame.P2.x) / 2.0, (frame.P1.y + frame.P2.y) / 2.0};
    const TDMatPoint midRight{(frame.P2.x + frame.P3.x) / 2.0, (frame.P2.y + frame.P3.y) / 2.0};
    const TDMatPoint midBottom{(frame.P3.x + frame.P4.x) / 2.0, (frame.P3.y + frame.P4.y) / 2.0};
    const TDMatPoint midLeft{(frame.P4.x + frame.P1.x) / 2.0, (frame.P4.y + frame.P1.y) / 2.0};

    pGE->DrawNode(frame.P1.x, frame.P1.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(frame.P2.x, frame.P2.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(frame.P3.x, frame.P3.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(frame.P4.x, frame.P4.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(midTop.x, midTop.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(midRight.x, midRight.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(midBottom.x, midBottom.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(midLeft.x, midLeft.y, NODE_NORMAL, resizeLock);
}

TDVecObjectType TDVecObject::GetType() const {
    return type_;
}

TDVecLineHitResult TDVecObject::HitTest(TDMatPoint point, double tolerance) const {
    (void)point;
    (void)tolerance;
    return {};
}

TDVecLineHitResult TDVecObject::HitTestNode(TDMatPoint point, double tolerance) const {
    (void)point;
    (void)tolerance;
    return {};
}

TDVecLineHitResult TDVecObject::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    (void)pGE;
    (void)point;
    (void)tolerancePixels;
    return {};
}

TDVecLineHitResult TDVecObject::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    (void)pGE;
    (void)point;
    (void)tolerancePixels;
    return {};
}

long TDVecObject::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    (void)pGE;
    (void)X;
    (void)Y;
    (void)Toleranz;
    return -1;
}

long TDVecObject::PointOnNode(TDMatPoint point, double tolerance) const {
    (void)point;
    (void)tolerance;
    return -1;
}

bool TDVecObject::MoveBy(double dx, double dy) {
    (void)dx;
    (void)dy;
    return false;
}

bool TDVecObject::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    (void)iNode;
    (void)X;
    (void)Y;
    (void)MatCPoint;
    return false;
}

bool TDVecObject::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    (void)MatPoint;
    (void)xScale;
    (void)yScale;
    return false;
}

bool TDVecObject::Rotate(TDMatPoint MatPoint, double nAngle) {
    (void)MatPoint;
    (void)nAngle;
    return false;
}

void TDVecObject::TransformToPoint(TDMatPoint MatPoint) {
    (void)MatPoint;
}

void TDVecObject::TransformToOrigin(TDMatPoint MatPoint) {
    (void)MatPoint;
}

std::uint32_t TDVecObject::TypeFourCC() const
{
    return 0;
}

void TDVecObject::WriteTo(VEDBinaryWriter& writer) const
{
    writer.WriteBool(selected_);
    writer.WriteEnum(type_);
    writer.WriteBool(lockResize_);
    writer.WriteBool(lockPosition_);
    writer.WriteBool(resizeProportional_);
    writer.WriteUInt32(0); // old layer id
    writer.WriteUInt32(color_);
}

void TDVecObject::ReadObjectStateFrom(VEDBinaryReader& reader)
{
    selected_ = reader.ReadBool();
    type_ = static_cast<TDVecObjectType>(reader.ReadEnum());
    lockResize_ = reader.ReadBool();
    lockPosition_ = reader.ReadBool();
    resizeProportional_ = reader.ReadBool();
    reader.ReadUInt32(); // old layer id, not present in the current core slice
    color_ = reader.ReadUInt32();
}

long TDVecObject::PointOnFrameNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || !GetSelect()) {
        return -1;
    }

    return pointOnFrameNode(GetFrame(), pGE, X, Y, Toleranz);
}

long TDVecObject::PointOnFrameNode(TDMatPoint point, double tolerance) const {
    if (!GetSelect()) {
        return -1;
    }

    return pointOnFrameNode(GetFrame(), point, tolerance);
}

bool TDVecObject::MoveFrameNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    (void)MatCPoint;
    if (GetLockResize()) {
        return false;
    }

    return moveFrameNodeFromFrame(*this, GetFrame(), iNode, X, Y);
}

long TDVecObject::PointOnScaleFrameNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || !GetSelect()) {
        return -1;
    }

    return pointOnFrameNode(GetScaleFrame(), pGE, X, Y, Toleranz);
}

long TDVecObject::PointOnScaleFrameNode(TDMatPoint point, double tolerance) const {
    if (!GetSelect()) {
        return -1;
    }

    return pointOnFrameNode(GetScaleFrame(), point, tolerance);
}

bool TDVecObject::MoveScaleFrameNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    (void)MatCPoint;
    if (GetLockResize()) {
        return false;
    }

    return moveFrameNodeFromFrame(*this, GetScaleFrame(), iNode, X, Y);
}

void TDVecObject::SetSelect(bool bSelect) {
    selected_ = bSelect;
}

bool TDVecObject::GetSelect() const {
    return selected_;
}

void TDVecObject::SetColor(TDRgbColor color) {
    color_ = color;
}

TDRgbColor TDVecObject::GetColor() const {
    return color_;
}

void TDVecObject::Initialize() {
}

void TDVecObject::SetLockResize(bool bLockResize) {
    lockResize_ = bLockResize;
}

bool TDVecObject::GetLockResize() const {
    return lockResize_;
}

void TDVecObject::SetResizeProportional(bool bValue) {
    resizeProportional_ = bValue;
}

bool TDVecObject::GetResizeProportional() const {
    return resizeProportional_;
}

void TDVecObject::SetLockPosition(bool bLockPosition) {
    lockPosition_ = bLockPosition;
}

bool TDVecObject::GetLockPosition() const {
    return lockPosition_;
}

bool TDVecObject::GetLockObject() const {
    return lockResize_ || lockPosition_;
}

void TDVecObject::SetParentModel(TDVecModel* pParentModel) {
    parentModel_ = pParentModel;
}

TDVecModel* TDVecObject::GetParentModel() const {
    return parentModel_;
}

void TDVecObject::SetType(TDVecObjectType eType) {
    type_ = eType;
}
