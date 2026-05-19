#pragma once

#include "vec_object.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class VEDBinaryReader;
class VEDBinaryWriter;

class TDVecGroup : public TDVecObject {
public:
    TDVecGroup();
    TDVecGroup(const TDVecGroup& group);
    TDVecGroup& operator=(const TDVecGroup& group);

    void Draw(TDGraphicEngine* pGE, bool bOutLine) const override;
    std::uint32_t TypeFourCC() const override;
    void WriteTo(VEDBinaryWriter& writer) const override;
    static std::uint32_t StreamFourCC();
    static std::unique_ptr<TDVecGroup> ReadFrom(VEDBinaryReader& reader);
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
    bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) override;
    bool MoveBy(double dx, double dy) override;
    bool ToScale(TDMatPoint MatPoint, double xScale, double yScale) override;
    bool Rotate(TDMatPoint MatPoint, double nAngle) override;
    void TransformToPoint(TDMatPoint MatPoint) override;
    void TransformToOrigin(TDMatPoint MatPoint) override;

    void AppendInGroup(TDVecObject* pObject);
    TDVecObject* GetFromGroup(long iObject) const;
    std::unique_ptr<TDVecObject> GetCopyFromGroup(long iObject) const;
    void ForEachStoredObject(const std::function<void(TDVecObject&)>& visitor);
    long CountObjectsInGroup() const;
    void FirstInitialize();
    void SetResolveLock(bool bResolveLock);
    bool GetResolveLock() const;

private:
    void CalculateBoundary() const;

    std::vector<std::unique_ptr<TDVecObject>> objectsInGroup_;
    double rotateAngle_;
    double xScale_;
    double yScale_;
    mutable TDMatPoint topLeftBoundary_;
    mutable TDMatPoint bottomRightBoundary_;
    TDMatPoint originPoint_;
    double xLength_;
    double yLength_;
    long objectInGroupCount_;
    bool resolveLock_;
    TDMatRect oldFrame_;
};
