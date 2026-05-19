#pragma once

#include "vec_object.h"

#include <cstdint>
#include <memory>

class VEDBinaryReader;
class VEDBinaryWriter;

class TDVecLine : public TDVecObject {
public:
    TDVecLine();
    TDVecLine(double x1, double y1, double x2, double y2);

    void Draw(TDGraphicEngine* pGE, bool bOutLine) const override;
    std::uint32_t TypeFourCC() const override;
    void WriteTo(VEDBinaryWriter& writer) const override;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecLine> ReadFrom(VEDBinaryReader& reader);
    void DrawNodes(TDGraphicEngine* pGE) const override;
    TDMatRect GetFrame() const override;
    TDMatPoint GetMidpoint() const override;
    std::unique_ptr<TDVecObject> Clone() const override;

    TDVecLineHitResult HitTest(TDMatPoint point, double tolerance) const override;
    TDVecLineHitResult HitTestLine(TDMatPoint point, double tolerance) const;
    TDVecLineHitResult HitTestNode(TDMatPoint point, double tolerance) const override;
    TDVecLineHitResult HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const override;
    TDVecLineHitResult HitTestLine(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const;
    TDVecLineHitResult HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const override;
    bool MoveBy(double dx, double dy) override;
    long PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const override;
    long PointOnNode(TDMatPoint point, double tolerance) const override;
    bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) override;
    bool ToScale(TDMatPoint MatPoint, double xScale, double yScale) override;
    bool Rotate(TDMatPoint MatPoint, double nAngle) override;
    void TransformToPoint(TDMatPoint MatPoint) override;
    void TransformToOrigin(TDMatPoint MatPoint) override;
    bool MoveNode(TDVecLineHitKind nodeKind, TDMatPoint point);

    void SetLine(double x1, double y1, double x2, double y2);
    void SetLine(const TDMatLine* pMatLine);
    TDMatLine GetLine() const;

private:
    double x1_;
    double y1_;
    double x2_;
    double y2_;
};
