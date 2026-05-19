#pragma once

#include "vec_graphic_engine.h"
#include "vec_maindef.h"
#include "vec_math_base.h"

#include <cstdint>
#include <memory>

class VEDBinaryWriter;
class VEDBinaryReader;
class TDVecModel;

enum class TDVecLineHitKind {
    None,
    Body,
    StartNode,
    EndNode,
    MidpointNode
};

struct TDVecLineHitResult {
    TDVecLineHitKind kind = TDVecLineHitKind::None;
    double distance = 0.0;

    bool IsHit() const;
    bool IsNodeHit() const;
};

class TDVecObject {
public:
    TDVecObject();
    explicit TDVecObject(VEDBinaryReader& reader);
    virtual ~TDVecObject() = default;

    virtual void Draw(TDGraphicEngine* pGE, bool bOutLine) const;
    virtual void DrawNodes(TDGraphicEngine* pGE) const = 0;
    virtual TDMatRect GetFrame() const = 0;
    // Operation-specific resize frame. Most objects use GetFrame(); ellipse
    // overrides this so Select/Scale can show a horizontal bounding frame.
    virtual TDMatRect GetScaleFrame() const;
    virtual TDMatPoint GetMidpoint() const = 0;
    virtual std::unique_ptr<TDVecObject> Clone() const = 0;
    virtual TDVecLineHitResult HitTest(TDMatPoint point, double tolerance) const;
    virtual TDVecLineHitResult HitTestNode(TDMatPoint point, double tolerance) const;
    virtual TDVecLineHitResult HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const;
    virtual TDVecLineHitResult HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const;
    virtual long PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const;
    virtual long PointOnNode(TDMatPoint point, double tolerance) const;
    virtual bool MoveBy(double dx, double dy);
    virtual bool MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint);
    virtual bool ToScale(TDMatPoint MatPoint, double xScale, double yScale);
    virtual bool Rotate(TDMatPoint MatPoint, double nAngle);
    virtual void TransformToPoint(TDMatPoint MatPoint);
    virtual void TransformToOrigin(TDMatPoint MatPoint);
    virtual std::uint32_t TypeFourCC() const;
    virtual void WriteTo(VEDBinaryWriter& writer) const;

    void DrawFrame(TDGraphicEngine* pGE) const;
    long PointOnFrameNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const;
    long PointOnFrameNode(TDMatPoint point, double tolerance) const;
    bool MoveFrameNode(long iNode, double X, double Y, TDMatPoint MatCPoint);
    // Frame-node helpers used by VOM_SELECTMOVE_OBJECT_SCALE_FRAME.
    long PointOnScaleFrameNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const;
    long PointOnScaleFrameNode(TDMatPoint point, double tolerance) const;
    bool MoveScaleFrameNode(long iNode, double X, double Y, TDMatPoint MatCPoint);

    TDVecObjectType GetType() const;
    void SetSelect(bool bSelect);
    bool GetSelect() const;
    void SetColor(TDRgbColor color);
    TDRgbColor GetColor() const;
    virtual void Initialize();
    void SetLockResize(bool bLockResize);
    bool GetLockResize() const;
    void SetResizeProportional(bool bValue);
    bool GetResizeProportional() const;
    void SetLockPosition(bool bLockPosition);
    bool GetLockPosition() const;
    bool GetLockObject() const;
    void SetParentModel(TDVecModel* pParentModel);
    TDVecModel* GetParentModel() const;

protected:
    void SetType(TDVecObjectType eType);
    void ReadObjectStateFrom(VEDBinaryReader& reader);

private:
    TDVecObjectType type_;
    bool selected_;
    bool lockResize_;
    bool resizeProportional_;
    bool lockPosition_;
    TDRgbColor color_;
    TDVecModel* parentModel_;
};
