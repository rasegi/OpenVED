#pragma once

#include "vec_object.h"
#include "vec_contur_point.h"

#include <cstdint>
#include <memory>
#include <vector>

class VEDBinaryReader;
class VEDBinaryWriter;

class TDVecPolyCurveArea : public TDVecObject {
public:
    TDVecPolyCurveArea();
    TDVecPolyCurveArea(const TDVecPolyCurveArea&) = default;
    TDVecPolyCurveArea& operator=(const TDVecPolyCurveArea&) = default;

    void Draw(TDGraphicEngine* pGE, bool bOutLine) const override;
    std::uint32_t TypeFourCC() const override;
    void WriteTo(VEDBinaryWriter& writer) const override;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecPolyCurveArea> ReadFrom(VEDBinaryReader& reader);
    void DrawNodes(TDGraphicEngine* pGE) const override;
    TDMatRect GetFrame() const override;
    TDMatPoint GetMidpoint() const override;
    std::unique_ptr<TDVecObject> Clone() const override;
    TDVecLineHitResult HitTest(TDMatPoint point, double tolerance) const override;
    TDVecLineHitResult HitTestNode(TDMatPoint point, double tolerance) const override;
    TDVecLineHitResult HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const override;
    TDVecLineHitResult HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const override;
    long PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const override;
    long PointOnNode(TDMatPoint point, double tolerance) const override;
    long PointAfterNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const;
    bool MoveBy(double dx, double dy) override;
    bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) override;
    bool ToScale(TDMatPoint MatPoint, double xScale, double yScale) override;
    bool Rotate(TDMatPoint MatPoint, double nAngle) override;
    void TransformToPoint(TDMatPoint MatPoint) override;
    void TransformToOrigin(TDMatPoint MatPoint) override;

    void SetRectangularLock(bool bValue);
    bool GetRectangularLock() const;
    void SetShowControls(bool bShowControls);
    bool GetShowControls() const;
    void SetShowPolygon(bool bShowPolygon);
    bool GetShowPolygon() const;
    void SetResolution(unsigned int nResolution);
    unsigned int GetResolution() const;
    bool ChangePointType(int iPoint);
    bool InsertPoint(int iPoint, const TDMatConturPoint& point);
    bool AppendPoint(const TDMatConturPoint& point);
    bool RemovePoint(int iPoint);
    bool ClearPoints();
    const TDMatConturPoint* GetPoint(int iPoint) const;
    int CountPoints() const;

private:
    std::vector<TDMatConturPoint> conturPoints_;
    bool rectangularLock_;
    unsigned int resolution_;
    bool showControls_;
    bool showPolygon_;
};
