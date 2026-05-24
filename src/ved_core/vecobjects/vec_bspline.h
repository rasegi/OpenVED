#pragma once

#include "vec_object.h"

#include <cstdint>
#include <memory>
#include <vector>

class VEDBinaryReader;
class VEDBinaryWriter;

class TDVecBSPLine : public TDVecObject {
public:
    TDVecBSPLine();
    TDVecBSPLine(const TDVecBSPLine&) = default;
    TDVecBSPLine& operator=(const TDVecBSPLine&) = default;

    void Draw(TDGraphicEngine* pGE, bool bOutLine) const override;
    std::uint32_t TypeFourCC() const override;
    void WriteTo(VEDBinaryWriter& writer) const override;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecBSPLine> ReadFrom(VEDBinaryReader& reader);
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
    bool MoveBy(double dx, double dy) override;
    bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) override;
    bool ToScale(TDMatPoint MatPoint, double xScale, double yScale) override;
    bool Rotate(TDMatPoint MatPoint, double nAngle) override;
    void TransformToPoint(TDMatPoint MatPoint) override;
    void TransformToOrigin(TDMatPoint MatPoint) override;

    void ComputeCurve() const;
    long PointOnControle(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const;
    long PointAfterControle(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const;
    void MoveControle(long iControle, double X, double Y);
    void SetShowControls(bool bShowControls);
    bool GetShowControls() const;
    void SetShowPolygon(bool bShowPolygon);
    bool GetShowPolygon() const;
    void SetDegree(int nDegree);
    unsigned int GetDegree() const;
    void SetResolution(unsigned int nResolution);
    unsigned int GetResolution() const;
    TDMatRect GetFrameMin() const;
    void GenerateKnot() const;
    bool InsertPoint(int iPoint, const TDMatPoint& point);
    bool AppendPoint(const TDMatPoint& point);
    bool RemovePoint(int iPoint);
    bool ClearPoints();
    const TDMatPoint* GetPoint(int iPoint) const;
    int CountPoints() const;

private:
    void FrameControlsCompute();
    void ComputeHeightAndWidth();
    void EnsureCurveComputed() const;
    void MarkCurveDirty();
    void ComputeBasisFuns(int i, double u) const;
    TDMatPoint GetCurvePoint(double u) const;

    TDMatPointsArray controls_;
    unsigned int degree_;
    unsigned int resolution_;
    bool showControls_;
    bool showPolygon_;
    int leftControl_;
    int topControl_;
    int rightControl_;
    int bottomControl_;
    double height_;
    double width_;
    mutable TDMatPointsArray curve_;
    mutable std::vector<double> knotVector_;
    mutable std::vector<double> basisFuns_;
    mutable unsigned int numKnot_;
    mutable double upperMax_;
    mutable bool curveDirty_ = true;
    mutable std::vector<double> basisLeft_;
    mutable std::vector<double> basisRight_;
};
