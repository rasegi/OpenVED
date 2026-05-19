#include "vec_ellipse.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

TDVecEllipse::TDVecEllipse()
    : TDVecEllipse(0.0, 0.0, 0.0, 0.0, 0.0) {
}

TDVecEllipse::TDVecEllipse(double xCenter, double yCenter, double xRadius, double yRadius, double angle)
    : xCenter_(xCenter),
      yCenter_(yCenter),
      xRadius_(xRadius),
      yRadius_(yRadius),
      angle_(angle) {
    SetType(VECOBJ_ELL);
}

std::uint32_t TDVecEllipse::StreamFourCC()
{
    return VEDMakeFourCC('e', 'l', 'l', 'p');
}

std::uint32_t TDVecEllipse::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecEllipse> TDVecEllipse::ReadFrom(VEDBinaryReader& reader)
{
    auto ellipse = std::make_unique<TDVecEllipse>();
    ellipse->ReadObjectStateFrom(reader);
    ellipse->xCenter_ = reader.ReadDouble();
    ellipse->yCenter_ = reader.ReadDouble();
    ellipse->xRadius_ = reader.ReadDouble();
    ellipse->yRadius_ = reader.ReadDouble();
    ellipse->angle_ = reader.ReadDouble();
    return reader.Ok() ? std::move(ellipse) : nullptr;
}

void TDVecEllipse::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteDouble(xCenter_);
    writer.WriteDouble(yCenter_);
    writer.WriteDouble(xRadius_);
    writer.WriteDouble(yRadius_);
    writer.WriteDouble(angle_);
}

void TDVecEllipse::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    TDMatEllipse params{xCenter_, yCenter_, xRadius_, yRadius_, angle_};
    pGE->DrawEllipse(&params, bOutLine);
}

void TDVecEllipse::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE) {
        return;
    }

    const TDMatRect frame = GetFrame();
    const bool resizeLock = GetLockResize();
    const TDNodeType nodeType = std::fabs(xRadius_ - yRadius_) <= 0.000001 ? NODE_PROPORTIONAL : NODE_NORMAL;
    const TDMatPoint midTop{(frame.P1.x + frame.P2.x) / 2.0, (frame.P1.y + frame.P2.y) / 2.0};
    const TDMatPoint midRight{(frame.P2.x + frame.P3.x) / 2.0, (frame.P2.y + frame.P3.y) / 2.0};
    const TDMatPoint midBottom{(frame.P3.x + frame.P4.x) / 2.0, (frame.P3.y + frame.P4.y) / 2.0};
    const TDMatPoint midLeft{(frame.P4.x + frame.P1.x) / 2.0, (frame.P4.y + frame.P1.y) / 2.0};

    pGE->DrawNode(frame.P1.x, frame.P1.y, nodeType, resizeLock);
    pGE->DrawNode(frame.P2.x, frame.P2.y, nodeType, resizeLock);
    pGE->DrawNode(frame.P3.x, frame.P3.y, nodeType, resizeLock);
    pGE->DrawNode(frame.P4.x, frame.P4.y, nodeType, resizeLock);
    pGE->DrawNode(midTop.x, midTop.y, nodeType, resizeLock);
    pGE->DrawNode(midRight.x, midRight.y, nodeType, resizeLock);
    pGE->DrawNode(midBottom.x, midBottom.y, nodeType, resizeLock);
    pGE->DrawNode(midLeft.x, midLeft.y, nodeType, resizeLock);
    pGE->DrawNode(xCenter_, yCenter_, NODE_MIDPOINT, GetLockPosition());
}

TDMatRect TDVecEllipse::GetFrame() const {
    const double c = cosD(angle_);
    const double s = sinD(angle_);
    return {
        {c * (-xRadius_) + s * (-yRadius_) + xCenter_, -s * (-xRadius_) + c * (-yRadius_) + yCenter_},
        {c * xRadius_ + s * (-yRadius_) + xCenter_, -s * xRadius_ + c * (-yRadius_) + yCenter_},
        {c * xRadius_ + s * yRadius_ + xCenter_, -s * xRadius_ + c * yRadius_ + yCenter_},
        {c * (-xRadius_) + s * yRadius_ + xCenter_, -s * (-xRadius_) + c * yRadius_ + yCenter_}
    };
}

TDMatRect TDVecEllipse::GetScaleFrame() const {
    const double c = cosD(angle_);
    const double s = sinD(angle_);
    // Select/Scale uses a horizontal bounding frame for consistency with
    // polygonal objects. GetFrame() stays angle-oriented for normal ellipse
    // nodes and Borland-compatible geometry.
    const double xExtent = std::sqrt((xRadius_ * c * xRadius_ * c) + (yRadius_ * s * yRadius_ * s));
    const double yExtent = std::sqrt((xRadius_ * s * xRadius_ * s) + (yRadius_ * c * yRadius_ * c));
    return makeFrameFromBounds(xCenter_ - xExtent, yCenter_ - yExtent, xCenter_ + xExtent, yCenter_ + yExtent);
}

TDMatPoint TDVecEllipse::GetMidpoint() const {
    return {xCenter_, yCenter_};
}

std::unique_ptr<TDVecObject> TDVecEllipse::Clone() const {
    return std::make_unique<TDVecEllipse>(*this);
}

void TDVecEllipse::Initialize() {
    while (angle_ > 360.0 || angle_ < -360.0) {
        if (angle_ > 360.0) {
            angle_ -= 360.0;
        }
        if (angle_ < -360.0) {
            angle_ += 360.0;
        }
    }

    if (IsDiagonal() &&
        !MatBelike2Double(angle_, 0.0, 4) &&
        !MatBelike2Double(angle_, 360.0, 4) &&
        !MatBelike2Double(angle_, 180.0, 4)) {
        std::swap(xRadius_, yRadius_);
        angle_ = 0.0;
    }

    SetResizeProportional(MatBelike2Double(xRadius_, yRadius_, 4));
}

TDVecLineHitResult TDVecEllipse::HitTest(TDMatPoint point, double tolerance) const {
    const TDVecLineHitResult nodeHit = HitTestNode(point, tolerance);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }

    TDMatPoint previous = ellipsePoint(xCenter_, yCenter_, xRadius_, yRadius_, angle_, 0.0);
    double bestDistance = std::numeric_limits<double>::infinity();
    for (int segment = 1; segment <= kEllipseHitSegments; ++segment) {
        const double radians = (2.0 * M_PI * static_cast<double>(segment)) / static_cast<double>(kEllipseHitSegments);
        const TDMatPoint current = ellipsePoint(xCenter_, yCenter_, xRadius_, yRadius_, angle_, radians);
        bestDistance = std::min(bestDistance, distancePointToSegment(point, previous, current));
        previous = current;
    }
    if (bestDistance <= tolerance) {
        return {TDVecLineHitKind::Body, bestDistance};
    }
    return {};
}

TDVecLineHitResult TDVecEllipse::HitTestNode(TDMatPoint point, double tolerance) const {
    if (tolerance < 0.0) {
        return {};
    }

    const TDMatPoint midpoint = GetMidpoint();
    const double centerDistance = distance(point, midpoint);
    if (centerDistance <= tolerance) {
        return {TDVecLineHitKind::MidpointNode, centerDistance};
    }

    const TDMatRect frame = GetFrame();
    const std::array candidates{
        distance(point, frame.P1),
        distance(point, frame.P2),
        distance(point, frame.P3),
        distance(point, frame.P4)
    };
    const double bestDistance = *std::min_element(candidates.begin(), candidates.end());
    if (bestDistance <= tolerance) {
        return {TDVecLineHitKind::Body, bestDistance};
    }
    return {};
}

TDVecLineHitResult TDVecEllipse::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const TDVecLineHitResult nodeHit = HitTestNode(pGE, point, tolerancePixels);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }
    if (!pGE || tolerancePixels < 0) {
        return {};
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(point.x));
    const double mouseY = static_cast<double>(pGE->RealToYPos(point.y));
    TDMatPoint previous = ellipsePoint(xCenter_, yCenter_, xRadius_, yRadius_, angle_, 0.0);
    double previousX = static_cast<double>(pGE->RealToXPos(previous.x));
    double previousY = static_cast<double>(pGE->RealToYPos(previous.y));
    double bestDistance = std::numeric_limits<double>::infinity();
    for (int segment = 1; segment <= kEllipseHitSegments; ++segment) {
        const double radians = (2.0 * M_PI * static_cast<double>(segment)) / static_cast<double>(kEllipseHitSegments);
        const TDMatPoint current = ellipsePoint(xCenter_, yCenter_, xRadius_, yRadius_, angle_, radians);
        const double currentX = static_cast<double>(pGE->RealToXPos(current.x));
        const double currentY = static_cast<double>(pGE->RealToYPos(current.y));
        bestDistance = std::min(bestDistance, distancePointToSegment(mouseX, mouseY, previousX, previousY, currentX, currentY));
        previousX = currentX;
        previousY = currentY;
    }
    if (bestDistance <= static_cast<double>(tolerancePixels)) {
        return {TDVecLineHitKind::Body, bestDistance};
    }
    return {};
}

TDVecLineHitResult TDVecEllipse::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    if (!pGE || tolerancePixels < 0) {
        return {};
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(point.x));
    const double mouseY = static_cast<double>(pGE->RealToYPos(point.y));
    const double centerDistance = std::hypot(static_cast<double>(pGE->RealToXPos(xCenter_)) - mouseX, static_cast<double>(pGE->RealToYPos(yCenter_)) - mouseY);
    if (centerDistance <= static_cast<double>(tolerancePixels)) {
        return {TDVecLineHitKind::MidpointNode, centerDistance};
    }
    return {};
}

long TDVecEllipse::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    return PointOnFrameNode(pGE, X, Y, Toleranz);
}

long TDVecEllipse::PointOnNode(TDMatPoint point, double tolerance) const {
    return PointOnFrameNode(point, tolerance);
}

bool TDVecEllipse::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    if (GetLockResize()) {
        return false;
    }

    double newXRadius = xRadius_;
    double newYRadius = yRadius_;
    if (IsDiagonal()) {
        switch (iNode) {
        case 0:
            newXRadius = xRadius_ - X;
            newYRadius = yRadius_ - Y;
            break;
        case 1:
            newYRadius = yRadius_ - Y;
            break;
        case 2:
            newXRadius = xRadius_ + X;
            newYRadius = yRadius_ - Y;
            break;
        case 3:
            newXRadius = xRadius_ + X;
            break;
        case 4:
            newXRadius = xRadius_ + X;
            newYRadius = yRadius_ + Y;
            break;
        case 5:
            newYRadius = yRadius_ + Y;
            break;
        case 6:
            newXRadius = xRadius_ - X;
            newYRadius = yRadius_ + Y;
            break;
        case 7:
            newXRadius = xRadius_ - X;
            break;
        default:
            return false;
        }
    } else {
        const TDMatPoint midpoint{xCenter_, yCenter_};
        TDMatPoint keyPoint;
        switch (iNode) {
        case 0:
        case 6: {
            const TDMatPoint axisPoint = GetNode(7);
            keyPoint = projectPointOnLine(midpoint, axisPoint, MatCPoint);
            newXRadius = MatDistance2Point(keyPoint, midpoint);
            newYRadius = MatDistance2Point(keyPoint, MatCPoint);
            break;
        }
        case 2:
        case 4: {
            const TDMatPoint axisPoint = GetNode(3);
            keyPoint = projectPointOnLine(midpoint, axisPoint, MatCPoint);
            newXRadius = MatDistance2Point(keyPoint, midpoint);
            newYRadius = MatDistance2Point(keyPoint, MatCPoint);
            break;
        }
        case 1: {
            const TDMatPoint axisPoint = GetNode(1);
            if (horizontalCut(midpoint, axisPoint, MatCPoint, keyPoint)) {
                newYRadius = MatDistance2Point(keyPoint, midpoint);
            }
            break;
        }
        case 3: {
            const TDMatPoint axisPoint = GetNode(3);
            if (verticalCut(midpoint, axisPoint, MatCPoint, keyPoint)) {
                newXRadius = MatDistance2Point(keyPoint, midpoint);
            }
            break;
        }
        case 5: {
            const TDMatPoint axisPoint = GetNode(5);
            if (horizontalCut(midpoint, axisPoint, MatCPoint, keyPoint)) {
                newYRadius = MatDistance2Point(keyPoint, midpoint);
            }
            break;
        }
        case 7: {
            const TDMatPoint axisPoint = GetNode(7);
            if (verticalCut(midpoint, axisPoint, MatCPoint, keyPoint)) {
                newXRadius = MatDistance2Point(keyPoint, midpoint);
            }
            break;
        }
        default:
            return false;
        }
    }

    ApplyMovedNodeRadii(newXRadius, newYRadius);
    return true;
}

TDMatPoint TDVecEllipse::GetNode(long iNode) const {
    return frameNode(GetFrame(), iNode);
}

void TDVecEllipse::ApplyMovedNodeRadii(double newXRadius, double newYRadius) {
    if (GetResizeProportional() &&
        !MatBelike2Double(xRadius_, 0.0, 4) &&
        !MatBelike2Double(yRadius_, 0.0, 4)) {
        const double xRadiusDelta = std::fabs(newXRadius - xRadius_);
        const double yRadiusDelta = std::fabs(newYRadius - yRadius_);
        const bool xDeltaZero = MatBelike2Double(xRadiusDelta, 0.0, 4);
        const bool yDeltaZero = MatBelike2Double(yRadiusDelta, 0.0, 4);
        double percent = 1.0;

        if (!xDeltaZero && !yDeltaZero) {
            percent = xRadiusDelta >= yRadiusDelta ? std::fabs(newXRadius) / xRadius_ : std::fabs(newYRadius) / yRadius_;
        } else if (!xDeltaZero && yDeltaZero) {
            percent = std::fabs(newXRadius) / xRadius_;
        } else if (xDeltaZero && !yDeltaZero) {
            percent = std::fabs(newYRadius) / yRadius_;
        }
        xRadius_ *= percent;
        yRadius_ *= percent;
    } else {
        xRadius_ = newXRadius;
        yRadius_ = newYRadius;
    }

    xRadius_ = std::fabs(xRadius_);
    yRadius_ = std::fabs(yRadius_);
}

void TDVecEllipse::Import(TDMatEllipse ellipse) {
    xCenter_ = ellipse.xCenter;
    yCenter_ = ellipse.yCenter;
    xRadius_ = ellipse.xRadius;
    yRadius_ = ellipse.yRadius;
    angle_ = ellipse.nAngle;
}

bool TDVecEllipse::IsDiagonal() const {
    return GetEllipse().IsDiagonal();
}

TDMatLine TDVecEllipse::GetBigAxis() const {
    const double c = cosD(angle_);
    const double s = sinD(angle_);
    if (xRadius_ >= yRadius_) {
        return {
            xCenter_ - c * xRadius_,
            yCenter_ + s * xRadius_,
            xCenter_ + c * xRadius_,
            yCenter_ - s * xRadius_
        };
    }
    return {
        xCenter_ - s * yRadius_,
        yCenter_ - c * yRadius_,
        xCenter_ + s * yRadius_,
        yCenter_ + c * yRadius_
    };
}

TDMatLine TDVecEllipse::GetSmallAxis() const {
    const double c = cosD(angle_);
    const double s = sinD(angle_);
    if (xRadius_ < yRadius_) {
        return {
            xCenter_ - c * xRadius_,
            yCenter_ + s * xRadius_,
            xCenter_ + c * xRadius_,
            yCenter_ - s * xRadius_
        };
    }
    return {
        xCenter_ - s * yRadius_,
        yCenter_ - c * yRadius_,
        xCenter_ + s * yRadius_,
        yCenter_ + c * yRadius_
    };
}

TDMatEllipse TDVecEllipse::GetEllipse() const {
    return {xCenter_, yCenter_, xRadius_, yRadius_, angle_};
}

bool TDVecEllipse::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    xCenter_ += dx;
    yCenter_ += dy;
    return true;
}

bool TDVecEllipse::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (GetLockResize()) {
        return false;
    }

    const double rxRx = xRadius_ * xRadius_;
    const double ryRy = yRadius_ * yRadius_;
    const double sxSx = xScale * xScale;
    const double sySy = yScale * yScale;

    if (rxRx * ryRy * sxSx * sySy > 0.0) {
        double cosCos = cosD(angle_) * cosD(angle_);
        double sinSin = sinD(angle_) * sinD(angle_);

        double a = (cosCos / rxRx) + (sinSin / ryRy);
        double b = (cosCos / ryRy) + (sinSin / rxRx);
        double c = (rxRx - ryRy) / (rxRx * ryRy) * sinD(2 * angle_);

        a = a / sxSx;
        b = b / sySy;
        c = c / (xScale * yScale);

        const double bMinusA = b - a;
        if (std::fabs(bMinusA) > 0.0) {
            angle_ = 0.5 * atanD2(c, bMinusA);
        } else {
            if (std::fabs(c) > 0.0) {
                angle_ = 45.0;
            } else {
                angle_ = 0.0;
            }
        }

        cosCos = cosD(angle_) * cosD(angle_);
        sinSin = sinD(angle_) * sinD(angle_);
        const double sinCos = sinD(angle_) * cosD(angle_);

        const double p = a * cosCos + b * sinSin - c * sinCos;
        if (p > 0.0) {
            xRadius_ = 1 / std::sqrt(p);
        } else {
            xRadius_ = 0.0;
        }

        const double q = b * cosCos + a * sinSin + c * sinCos;
        if (q > 0.0) {
            yRadius_ = 1 / std::sqrt(q);
        } else {
            yRadius_ = 0.0;
        }
    } else {
        xRadius_ = std::fabs(xScale * xRadius_);
        yRadius_ = std::fabs(yScale * yRadius_);

        if (!(sxSx * sySy > 0.0)) {
            angle_ = 0.0;
        }
    }

    xCenter_ = (xScale * (xCenter_ - MatPoint.x)) + MatPoint.x;
    yCenter_ = (yScale * (yCenter_ - MatPoint.y)) + MatPoint.y;
    return true;
}

bool TDVecEllipse::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    TDMatPoint centerTrans;
    TDMatPoint centerRotated;
    if (!IsDiagonal()) {
        centerTrans.x = xCenter_ - MatPoint.x;
        centerTrans.y = yCenter_ - MatPoint.y;
        centerRotated.x = cosD(-nAngle) * centerTrans.x + sinD(-nAngle) * centerTrans.y;
        centerRotated.y = -sinD(-nAngle) * centerTrans.x + cosD(-nAngle) * centerTrans.y;
        xCenter_ = centerRotated.x + MatPoint.x;
        yCenter_ = centerRotated.y + MatPoint.y;
        angle_ -= nAngle;
    } else {
        const double dSin = sinD(nAngle);
        const double dCos = cosD(nAngle);
        if (dSin == 1.0 || dSin == -1.0 || dCos == 0.0) {
            centerTrans.x = xCenter_ - MatPoint.x;
            centerTrans.y = yCenter_ - MatPoint.y;
            centerRotated.x = cosD(-nAngle) * centerTrans.x + sinD(-nAngle) * centerTrans.y;
            centerRotated.y = -sinD(-nAngle) * centerTrans.x + cosD(-nAngle) * centerTrans.y;
            xCenter_ = centerRotated.x + MatPoint.x;
            yCenter_ = centerRotated.y + MatPoint.y;
            std::swap(xRadius_, yRadius_);
        } else {
            if (dSin != 0.0 || dCos != 1.0 || dCos != -1.0) {
                centerTrans.x = xCenter_ - MatPoint.x;
                centerTrans.y = yCenter_ - MatPoint.y;
                centerRotated.x = cosD(-nAngle) * centerTrans.x + sinD(-nAngle) * centerTrans.y;
                centerRotated.y = -sinD(-nAngle) * centerTrans.x + cosD(-nAngle) * centerTrans.y;
                xCenter_ = centerRotated.x + MatPoint.x;
                yCenter_ = centerRotated.y + MatPoint.y;
                angle_ -= nAngle;
            }
        }
    }
    return true;
}

void TDVecEllipse::TransformToPoint(TDMatPoint MatPoint) {
    xCenter_ -= MatPoint.x;
    yCenter_ -= MatPoint.y;
}

void TDVecEllipse::TransformToOrigin(TDMatPoint MatPoint) {
    xCenter_ += MatPoint.x;
    yCenter_ += MatPoint.y;
}

void TDVecEllipse::SetCenter(TDMatPoint centerPoint) {
    SetCenter(centerPoint.x, centerPoint.y);
}

void TDVecEllipse::SetCenter(double xCenter, double yCenter) {
    xCenter_ = xCenter;
    yCenter_ = yCenter;
}

TDMatPoint TDVecEllipse::GetCenter() const {
    return {xCenter_, yCenter_};
}

void TDVecEllipse::SetXRadius(double xRadius) {
    xRadius_ = xRadius;
}

double TDVecEllipse::GetXRadius() const {
    return xRadius_;
}

void TDVecEllipse::SetYRadius(double yRadius) {
    yRadius_ = yRadius;
}

double TDVecEllipse::GetYRadius() const {
    return yRadius_;
}

void TDVecEllipse::SetAngle(double angle) {
    angle_ = angle;
}

double TDVecEllipse::GetAngle() const {
    return angle_;
}
