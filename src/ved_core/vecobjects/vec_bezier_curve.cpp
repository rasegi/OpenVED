#include "vec_bezier_curve.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

TDVecBezierCurve::TDVecBezierCurve()
    : resolution_(20),
      showControls_(false),
      showPolygon_(false),
      leftControl_(0),
      topControl_(0),
      rightControl_(0),
      bottomControl_(0),
      height_(0.0),
      width_(0.0) {
    SetType(VECOBJ_BZC);
}

std::uint32_t TDVecBezierCurve::StreamFourCC()
{
    return VEDMakeFourCC('b', 'z', 'c', 'r');
}

std::uint32_t TDVecBezierCurve::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecBezierCurve> TDVecBezierCurve::ReadFrom(VEDBinaryReader& reader)
{
    auto curve = std::make_unique<TDVecBezierCurve>();
    curve->ReadObjectStateFrom(reader);
    curve->resolution_ = reader.ReadUInt32();
    curve->showControls_ = reader.ReadBool();
    curve->showPolygon_ = reader.ReadBool();
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return nullptr;
    }

    curve->controls_.clear();
    curve->controls_.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        TDMatPoint point;
        point.x = reader.ReadDouble();
        point.y = reader.ReadDouble();
        curve->controls_.push_back(point);
    }
    curve->ComputeCurve();
    curve->FrameControlsCompute();
    curve->ComputeHeightAndWidth();
    return reader.Ok() ? std::move(curve) : nullptr;
}

void TDVecBezierCurve::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteUInt32(resolution_);
    writer.WriteBool(showControls_);
    writer.WriteBool(showPolygon_);
    writer.WriteInt32(CountPoints());
    for(const TDMatPoint& point : controls_) {
        writer.WriteDouble(point.x);
        writer.WriteDouble(point.y);
    }
}

void TDVecBezierCurve::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE || controls_.empty()) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    if (GetShowPolygon()) {
        pGE->DrawConstructPolygon(&controls_);
    }

    ComputeCurve();
    pGE->DrawPolygon(&curve_, bOutLine);

    if (GetShowControls() && !bOutLine) {
        for (const TDMatPoint& point : controls_) {
            pGE->DrawNode(point.x, point.y, NODE_CONTROL, GetLockResize());
        }
    }
}

void TDVecBezierCurve::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE || controls_.empty()) {
        return;
    }

    const TDMatRect frame = GetFrame();
    const TDMatRect middle = frameMidEdges(frame);
    const bool resizeLock = GetLockResize();
    pGE->DrawNode(frame.P1.x, frame.P1.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(frame.P2.x, frame.P2.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(frame.P3.x, frame.P3.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(frame.P4.x, frame.P4.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(middle.P1.x, middle.P1.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(middle.P2.x, middle.P2.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(middle.P3.x, middle.P3.y, NODE_NORMAL, resizeLock);
    pGE->DrawNode(middle.P4.x, middle.P4.y, NODE_NORMAL, resizeLock);

    const TDMatPoint midpoint = GetMidpoint();
    pGE->DrawNode(midpoint.x, midpoint.y, NODE_MIDPOINT, GetLockPosition());
}

TDMatRect TDVecBezierCurve::GetFrame() const {
    return frameFromPoints(controls_);
}

TDMatPoint TDVecBezierCurve::GetMidpoint() const {
    const TDMatRect frame = GetFrame();
    return {(frame.P1.x + frame.P3.x) / 2.0, (frame.P1.y + frame.P3.y) / 2.0};
}

std::unique_ptr<TDVecObject> TDVecBezierCurve::Clone() const {
    return std::make_unique<TDVecBezierCurve>(*this);
}

TDVecLineHitResult TDVecBezierCurve::HitTest(TDMatPoint point, double tolerance) const {
    ComputeCurve();
    if (GetShowPolygon()) {
        const TDVecLineHitResult polygonHit = hitOpenPolyline(point, controls_, tolerance);
        if (polygonHit.IsHit()) {
            return polygonHit;
        }
    }
    return hitOpenPolyline(point, curve_, tolerance);
}

TDVecLineHitResult TDVecBezierCurve::HitTestNode(TDMatPoint point, double tolerance) const {
    const long node = PointOnNode(point, tolerance);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

TDVecLineHitResult TDVecBezierCurve::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    ComputeCurve();
    if (GetShowPolygon()) {
        const TDVecLineHitResult polygonHit = hitOpenPolyline(pGE, point, controls_, tolerancePixels);
        if (polygonHit.IsHit()) {
            return polygonHit;
        }
    }
    return hitOpenPolyline(pGE, point, curve_, tolerancePixels);
}

TDVecLineHitResult TDVecBezierCurve::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const long node = PointOnNode(pGE, point.x, point.y, tolerancePixels);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

long TDVecBezierCurve::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    return PointOnFrameNode(pGE, X, Y, Toleranz);
}

long TDVecBezierCurve::PointOnNode(TDMatPoint point, double tolerance) const {
    return PointOnFrameNode(point, tolerance);
}

bool TDVecBezierCurve::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatPoint& point : controls_) {
        point.x += dx;
        point.y += dy;
    }
    ComputeCurve();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBezierCurve::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    const bool moved = MoveFrameNode(iNode, X, Y, MatCPoint);
    if (moved) {
        ComputeCurve();
        FrameControlsCompute();
        ComputeHeightAndWidth();
    }
    return moved;
}

bool TDVecBezierCurve::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (GetLockResize()) {
        return false;
    }

    TransformToPoint(MatPoint);
    for (TDMatPoint& control : controls_) {
        control.x = xScale * control.x;
        control.y = yScale * control.y;
    }
    for (TDMatPoint& curvePoint : curve_) {
        curvePoint.x = xScale * curvePoint.x;
        curvePoint.y = yScale * curvePoint.y;
    }
    TransformToOrigin(MatPoint);
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBezierCurve::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatPoint& control : controls_) {
        const TDMatPoint trans{control.x - MatPoint.x, control.y - MatPoint.y};
        control.x = cosD(nAngle) * trans.x - sinD(nAngle) * trans.y + MatPoint.x;
        control.y = sinD(nAngle) * trans.x + cosD(nAngle) * trans.y + MatPoint.y;
    }
    ComputeCurve();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

void TDVecBezierCurve::TransformToPoint(TDMatPoint MatPoint) {
    for (TDMatPoint& control : controls_) {
        control.x -= MatPoint.x;
        control.y -= MatPoint.y;
    }
    for (TDMatPoint& curvePoint : curve_) {
        curvePoint.x -= MatPoint.x;
        curvePoint.y -= MatPoint.y;
    }
}

void TDVecBezierCurve::TransformToOrigin(TDMatPoint MatPoint) {
    for (TDMatPoint& control : controls_) {
        control.x += MatPoint.x;
        control.y += MatPoint.y;
    }
    for (TDMatPoint& curvePoint : curve_) {
        curvePoint.x += MatPoint.x;
        curvePoint.y += MatPoint.y;
    }
}

void TDVecBezierCurve::ComputeCurve() const {
    curve_.clear();
    Bezier(&controls_, static_cast<int>(resolution_), &curve_);
}

long TDVecBezierCurve::PointOnControle(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE) {
        return -1;
    }

    const long xMouse = pGE->RealToXPos(X);
    const long yMouse = pGE->RealToYPos(Y);
    for (std::size_t i = 0; i < controls_.size(); ++i) {
        const TDMatPoint& point = controls_[i];
        const double distanceToControl = std::hypot(
            static_cast<double>(pGE->RealToXPos(point.x) - xMouse),
            static_cast<double>(pGE->RealToYPos(point.y) - yMouse));
        if (distanceToControl <= static_cast<double>(Toleranz)) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

void TDVecBezierCurve::MoveControle(long iControle, double X, double Y) {
    if (iControle < 0 || iControle >= CountPoints()) {
        return;
    }

    TDMatPoint& point = controls_[static_cast<std::size_t>(iControle)];
    point.x += X;
    point.y += Y;
    ComputeCurve();
    FrameControlsCompute();
    ComputeHeightAndWidth();
}

void TDVecBezierCurve::SetShowControls(bool bShowControls) {
    showControls_ = bShowControls;
}

bool TDVecBezierCurve::GetShowControls() const {
    return showControls_;
}

void TDVecBezierCurve::SetShowPolygon(bool bShowPolygon) {
    showPolygon_ = bShowPolygon;
}

bool TDVecBezierCurve::GetShowPolygon() const {
    return showPolygon_;
}

void TDVecBezierCurve::SetResolution(unsigned int nResolution) {
    resolution_ = nResolution != 0 ? nResolution : 20;
}

unsigned int TDVecBezierCurve::GetResolution() const {
    return resolution_;
}

bool TDVecBezierCurve::InsertPoint(int iPoint, const TDMatPoint& point) {
    if (GetLockResize() || iPoint < 0 || iPoint > CountPoints()) {
        return false;
    }

    controls_.insert(controls_.begin() + iPoint, point);
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBezierCurve::AppendPoint(const TDMatPoint& point) {
    controls_.push_back(point);
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBezierCurve::RemovePoint(int iPoint) {
    if (GetLockResize() || controls_.size() < 4 || iPoint < 0 || iPoint >= CountPoints()) {
        return false;
    }

    controls_.erase(controls_.begin() + iPoint);
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBezierCurve::ClearPoints() {
    controls_.clear();
    curve_.clear();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

const TDMatPoint* TDVecBezierCurve::GetPoint(int iPoint) const {
    if (iPoint < 0 || iPoint >= CountPoints()) {
        return nullptr;
    }
    return &controls_[static_cast<std::size_t>(iPoint)];
}

int TDVecBezierCurve::CountPoints() const {
    return static_cast<int>(controls_.size());
}

void TDVecBezierCurve::FrameControlsCompute() {
    if (controls_.empty()) {
        leftControl_ = 0;
        topControl_ = 0;
        rightControl_ = 0;
        bottomControl_ = 0;
        return;
    }

    double xLeft = controls_.front().x;
    double yTop = controls_.front().y;
    double xRight = controls_.front().x;
    double yBottom = controls_.front().y;
    leftControl_ = 0;
    topControl_ = 0;
    rightControl_ = 0;
    bottomControl_ = 0;
    for (std::size_t i = 1; i < controls_.size(); ++i) {
        const TDMatPoint& point = controls_[i];
        if (xLeft > point.x) {
            xLeft = point.x;
            leftControl_ = static_cast<int>(i);
        }
        if (yTop > point.y) {
            yTop = point.y;
            topControl_ = static_cast<int>(i);
        }
    }
    for (std::size_t i = 0; i < controls_.size(); ++i) {
        const TDMatPoint& point = controls_[i];
        if (xRight < point.x) {
            xRight = point.x;
            rightControl_ = static_cast<int>(i);
        }
        if (yBottom < point.y) {
            yBottom = point.y;
            bottomControl_ = static_cast<int>(i);
        }
    }
}

void TDVecBezierCurve::ComputeHeightAndWidth() {
    const TDMatRect frame = GetFrame();
    width_ = frame.P2.x - frame.P1.x;
    height_ = frame.P3.y - frame.P1.y;
}
