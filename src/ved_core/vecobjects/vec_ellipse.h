#pragma once

#include "vec_object.h"

#include <cstdint>
#include <memory>

class VEDBinaryReader;
class VEDBinaryWriter;

class TDVecEllipse : public TDVecObject {
public:
    TDVecEllipse();
    TDVecEllipse(double xCenter, double yCenter, double xRadius, double yRadius, double angle = 0.0);

    void Draw(TDGraphicEngine* pGE, bool bOutLine) const override;
    std::uint32_t TypeFourCC() const override;
    void WriteTo(VEDBinaryWriter& writer) const override;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecEllipse> ReadFrom(VEDBinaryReader& reader);
    void DrawNodes(TDGraphicEngine* pGE) const override;
    TDMatRect GetFrame() const override;
    TDMatRect GetScaleFrame() const override;
    TDMatPoint GetMidpoint() const override;
    std::unique_ptr<TDVecObject> Clone() const override;
    void Initialize() override;
    TDVecLineHitResult HitTest(TDMatPoint point, double tolerance) const override;
    TDVecLineHitResult HitTestNode(TDMatPoint point, double tolerance) const override;
    TDVecLineHitResult HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const override;
    TDVecLineHitResult HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const override;
    long PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const override;
    long PointOnNode(TDMatPoint point, double tolerance) const override;
    bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) override;

    void Import(TDMatEllipse ellipse);
    bool IsDiagonal() const;
    TDMatLine GetBigAxis() const;
    TDMatLine GetSmallAxis() const;
    TDMatEllipse GetEllipse() const;
    bool MoveBy(double dx, double dy) override;
    bool ToScale(TDMatPoint MatPoint, double xScale, double yScale) override;
    bool Rotate(TDMatPoint MatPoint, double nAngle) override;
    void TransformToPoint(TDMatPoint MatPoint) override;
    void TransformToOrigin(TDMatPoint MatPoint) override;

    void SetCenter(TDMatPoint centerPoint);
    void SetCenter(double xCenter, double yCenter);
    TDMatPoint GetCenter() const;
    void SetXRadius(double xRadius);
    double GetXRadius() const;
    void SetYRadius(double yRadius);
    double GetYRadius() const;
    void SetAngle(double angle);
    double GetAngle() const;

private:
    TDMatPoint GetNode(long iNode) const;
    void ApplyMovedNodeRadii(double newXRadius, double newYRadius);

    double xCenter_;
    double yCenter_;
    double xRadius_;
    double yRadius_;
    double angle_;
};
