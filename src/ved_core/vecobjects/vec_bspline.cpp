#include "vec_bspline.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

TDVecBSPLine::TDVecBSPLine()
    : degree_(2),
      resolution_(50),
      showControls_(false),
      showPolygon_(false),
      leftControl_(0),
      topControl_(0),
      rightControl_(0),
      bottomControl_(0),
      height_(0.0),
      width_(0.0),
      numKnot_(0),
      upperMax_(0.0) {
    SetType(VECOBJ_BSPLINE);
}

std::uint32_t TDVecBSPLine::StreamFourCC()
{
    return VEDMakeFourCC('b', 's', 'p', 'l');
}

std::uint32_t TDVecBSPLine::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecBSPLine> TDVecBSPLine::ReadFrom(VEDBinaryReader& reader)
{
    auto bspline = std::make_unique<TDVecBSPLine>();
    bspline->ReadObjectStateFrom(reader);
    bspline->degree_ = reader.ReadUInt32();
    bspline->resolution_ = reader.ReadUInt32();
    bspline->showControls_ = reader.ReadBool();
    bspline->showPolygon_ = reader.ReadBool();
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return nullptr;
    }

    bspline->controls_.clear();
    bspline->controls_.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        TDMatPoint point;
        point.x = reader.ReadDouble();
        point.y = reader.ReadDouble();
        bspline->controls_.push_back(point);
    }
    bspline->GenerateKnot();
    bspline->ComputeCurve();
    bspline->FrameControlsCompute();
    bspline->ComputeHeightAndWidth();
    return reader.Ok() ? std::move(bspline) : nullptr;
}

void TDVecBSPLine::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteUInt32(degree_);
    writer.WriteUInt32(resolution_);
    writer.WriteBool(showControls_);
    writer.WriteBool(showPolygon_);
    writer.WriteInt32(CountPoints());
    for(const TDMatPoint& point : controls_) {
        writer.WriteDouble(point.x);
        writer.WriteDouble(point.y);
    }
}

void TDVecBSPLine::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE || controls_.empty()) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    if (GetShowPolygon()) {
        pGE->DrawConstructPolygon(&controls_);
    }

    EnsureCurveComputed();
    pGE->DrawPolygon(&curve_, bOutLine);

    if (GetShowControls() && !bOutLine) {
        for (const TDMatPoint& point : controls_) {
            pGE->DrawNode(point.x, point.y, NODE_CONTROL, GetLockResize());
        }
    }
}

void TDVecBSPLine::DrawNodes(TDGraphicEngine* pGE) const {
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

TDMatRect TDVecBSPLine::GetFrame() const {
    return frameFromPoints(controls_);
}

TDMatPoint TDVecBSPLine::GetMidpoint() const {
    const TDMatRect frame = GetFrame();
    return {(frame.P1.x + frame.P3.x) / 2.0, (frame.P1.y + frame.P3.y) / 2.0};
}

std::unique_ptr<TDVecObject> TDVecBSPLine::Clone() const {
    return std::make_unique<TDVecBSPLine>(*this);
}

TDVecLineHitResult TDVecBSPLine::HitTest(TDMatPoint point, double tolerance) const {
    EnsureCurveComputed();
    if (GetShowPolygon()) {
        const TDVecLineHitResult polygonHit = hitOpenPolyline(point, controls_, tolerance);
        if (polygonHit.IsHit()) {
            return polygonHit;
        }
    }
    return hitOpenPolyline(point, curve_, tolerance);
}

TDVecLineHitResult TDVecBSPLine::HitTestNode(TDMatPoint point, double tolerance) const {
    const long node = PointOnNode(point, tolerance);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

TDVecLineHitResult TDVecBSPLine::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    EnsureCurveComputed();
    if (GetShowPolygon()) {
        const TDVecLineHitResult polygonHit = hitOpenPolyline(pGE, point, controls_, tolerancePixels);
        if (polygonHit.IsHit()) {
            return polygonHit;
        }
    }
    return hitOpenPolyline(pGE, point, curve_, tolerancePixels);
}

TDVecLineHitResult TDVecBSPLine::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const long node = PointOnNode(pGE, point.x, point.y, tolerancePixels);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

long TDVecBSPLine::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    return PointOnFrameNode(pGE, X, Y, Toleranz);
}

long TDVecBSPLine::PointOnNode(TDMatPoint point, double tolerance) const {
    return PointOnFrameNode(point, tolerance);
}

bool TDVecBSPLine::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatPoint& point : controls_) {
        point.x += dx;
        point.y += dy;
    }
    MarkCurveDirty();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBSPLine::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    const bool moved = MoveFrameNode(iNode, X, Y, MatCPoint);
    if (moved) {
        MarkCurveDirty();
        FrameControlsCompute();
        ComputeHeightAndWidth();
    }
    return moved;
}

bool TDVecBSPLine::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (GetLockResize()) {
        return false;
    }

    TransformToPoint(MatPoint);
    for (TDMatPoint& control : controls_) {
        control.x = xScale * control.x;
        control.y = yScale * control.y;
    }
    TransformToOrigin(MatPoint);
    MarkCurveDirty();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBSPLine::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatPoint& control : controls_) {
        const TDMatPoint trans{control.x - MatPoint.x, control.y - MatPoint.y};
        control.x = cosD(nAngle) * trans.x - sinD(nAngle) * trans.y + MatPoint.x;
        control.y = sinD(nAngle) * trans.x + cosD(nAngle) * trans.y + MatPoint.y;
    }
    MarkCurveDirty();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

void TDVecBSPLine::TransformToPoint(TDMatPoint MatPoint) {
    for (TDMatPoint& control : controls_) {
        control.x -= MatPoint.x;
        control.y -= MatPoint.y;
    }
    MarkCurveDirty();
}

void TDVecBSPLine::TransformToOrigin(TDMatPoint MatPoint) {
    for (TDMatPoint& control : controls_) {
        control.x += MatPoint.x;
        control.y += MatPoint.y;
    }
    MarkCurveDirty();
}

void TDVecBSPLine::EnsureCurveComputed() const {
    if (!curveDirty_) {
        return;
    }
    ComputeCurve();
}

void TDVecBSPLine::MarkCurveDirty() {
    curveDirty_ = true;
}

void TDVecBSPLine::ComputeCurve() const {
    curveDirty_ = false;
    curve_.clear();
    if (controls_.size() < static_cast<std::size_t>(degree_ + 1)) {
        return;
    }

    if (knotVector_.empty()) {
        GenerateKnot();
    }

    curve_.reserve(resolution_ + 3);
    curve_.push_back(controls_.front());
    const double resolution = upperMax_ / resolution_;
    for (double u = 0.0; u <= upperMax_; u += resolution) {
        curve_.push_back(GetCurvePoint(u));
    }
    curve_.push_back(controls_.back());
}

long TDVecBSPLine::PointOnControle(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
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

long TDVecBSPLine::PointAfterControle(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || controls_.empty()) {
        return -1;
    }

    const TDMatPoint mouseScreen{
        static_cast<double>(pGE->RealToXPos(X)),
        static_cast<double>(pGE->RealToYPos(Y))
    };
    for (std::size_t i = 1; i < controls_.size(); ++i) {
        const TDMatPoint start{
            static_cast<double>(pGE->RealToXPos(controls_[i - 1].x)),
            static_cast<double>(pGE->RealToYPos(controls_[i - 1].y))
        };
        const TDMatPoint end{
            static_cast<double>(pGE->RealToXPos(controls_[i].x)),
            static_cast<double>(pGE->RealToYPos(controls_[i].y))
        };
        if (distancePointToSegment(mouseScreen, start, end) <= static_cast<double>(Toleranz)) {
            return static_cast<long>(i - 1);
        }
    }
    return -1;
}

void TDVecBSPLine::MoveControle(long iControle, double X, double Y) {
    if (iControle < 0 || iControle >= CountPoints()) {
        return;
    }

    TDMatPoint& point = controls_[static_cast<std::size_t>(iControle)];
    point.x += X;
    point.y += Y;
    MarkCurveDirty();
    FrameControlsCompute();
    ComputeHeightAndWidth();
}

void TDVecBSPLine::SetShowControls(bool bShowControls) {
    showControls_ = bShowControls;
}

bool TDVecBSPLine::GetShowControls() const {
    return showControls_;
}

void TDVecBSPLine::SetShowPolygon(bool bShowPolygon) {
    showPolygon_ = bShowPolygon;
}

bool TDVecBSPLine::GetShowPolygon() const {
    return showPolygon_;
}

void TDVecBSPLine::SetDegree(int nDegree) {
    degree_ = nDegree != 0 ? static_cast<unsigned int>(nDegree) : 2;
    GenerateKnot();
    MarkCurveDirty();
}

unsigned int TDVecBSPLine::GetDegree() const {
    return degree_;
}

void TDVecBSPLine::SetResolution(unsigned int nResolution) {
    resolution_ = nResolution != 0 ? nResolution : 50;
    MarkCurveDirty();
}

unsigned int TDVecBSPLine::GetResolution() const {
    return resolution_;
}

TDMatRect TDVecBSPLine::GetFrameMin() const {
    EnsureCurveComputed();
    return frameFromPoints(curve_);
}

void TDVecBSPLine::GenerateKnot() const {
    double knotVal = 1.0;
    const double zero = 0.0;

    numKnot_ = static_cast<unsigned int>(controls_.size()) + degree_ + 1;
    knotVector_.clear();
    for (int i = 0; i <= static_cast<int>(degree_); ++i) {
        knotVector_.push_back(zero);
    }
    for (int i = static_cast<int>(degree_) + 1; i < CountPoints(); ++i) {
        knotVector_.push_back(knotVal);
        knotVal = knotVal + 1.0;
    }
    for (int i = CountPoints(); i <= static_cast<int>(numKnot_); ++i) {
        knotVector_.push_back(knotVal);
    }
    upperMax_ = knotVal;
}

bool TDVecBSPLine::InsertPoint(int iPoint, const TDMatPoint& point) {
    if (GetLockResize() || iPoint < 0 || iPoint > CountPoints()) {
        return false;
    }

    controls_.insert(controls_.begin() + iPoint, point);
    GenerateKnot();
    MarkCurveDirty();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBSPLine::AppendPoint(const TDMatPoint& point) {
    controls_.push_back(point);
    GenerateKnot();
    MarkCurveDirty();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBSPLine::RemovePoint(int iPoint) {
    if (GetLockResize() || controls_.size() < 4 || iPoint < 0 || iPoint >= CountPoints()) {
        return false;
    }

    controls_.erase(controls_.begin() + iPoint);
    GenerateKnot();
    MarkCurveDirty();
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

bool TDVecBSPLine::ClearPoints() {
    controls_.clear();
    curve_.clear();
    knotVector_.clear();
    basisFuns_.clear();
    basisLeft_.clear();
    basisRight_.clear();
    numKnot_ = 0;
    upperMax_ = 0.0;
    curveDirty_ = true;
    FrameControlsCompute();
    ComputeHeightAndWidth();
    return true;
}

const TDMatPoint* TDVecBSPLine::GetPoint(int iPoint) const {
    if (iPoint < 0 || iPoint >= CountPoints()) {
        return nullptr;
    }
    return &controls_[static_cast<std::size_t>(iPoint)];
}

int TDVecBSPLine::CountPoints() const {
    return static_cast<int>(controls_.size());
}

void TDVecBSPLine::FrameControlsCompute() {
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

void TDVecBSPLine::ComputeHeightAndWidth() {
    const TDMatRect frame = GetFrame();
    width_ = frame.P2.x - frame.P1.x;
    height_ = frame.P3.y - frame.P1.y;
}

TDMatPoint TDVecBSPLine::GetCurvePoint(double u) const {
    int span = 0;
    TDMatPoint result;

    if (knotVector_.empty() || controls_.empty()) {
        return result;
    }

    if (u > knotVector_.back()) {
        return controls_.back();
    }
    if (u == 0.0) {
        return controls_.front();
    }

    while (span + 1 < static_cast<int>(knotVector_.size()) && u > knotVector_[static_cast<std::size_t>(span + 1)]) {
        span++;
    }
    ComputeBasisFuns(span, u);

    result.x = 0.0;
    result.y = 0.0;
    for (unsigned int j = 0; j <= degree_; j++) {
        const int controlIndex = span - static_cast<int>(degree_) + static_cast<int>(j);
        if (controlIndex < 0 || controlIndex >= CountPoints() || j >= basisFuns_.size()) {
            continue;
        }
        const TDMatPoint& control = controls_[static_cast<std::size_t>(controlIndex)];
        result.x += basisFuns_[j] * control.x;
        result.y += basisFuns_[j] * control.y;
    }
    return result;
}

void TDVecBSPLine::ComputeBasisFuns(int i, double u) const {
    const std::size_t size = static_cast<std::size_t>(degree_) + 2;
    basisRight_.assign(size, 0.0);
    basisLeft_.assign(size, 0.0);
    basisFuns_.assign(size, 0.0);

    basisFuns_[0] = 1.0;
    for (unsigned int j = 1; j <= degree_; j++) {
        basisLeft_[j] = u - knotVector_[static_cast<std::size_t>(i + 1 - static_cast<int>(j))];
        basisRight_[j] = knotVector_[static_cast<std::size_t>(i + static_cast<int>(j))] - u;
        double saved = 0.0;
        for (unsigned int r = 0; r < j; r++) {
            const double temp = basisFuns_[r] / (basisRight_[r + 1] + basisLeft_[j - r]);
            basisFuns_[r] = saved + basisRight_[r + 1] * temp;
            saved = basisLeft_[j - r] * temp;
        }
        basisFuns_[j] = saved;
    }
}
