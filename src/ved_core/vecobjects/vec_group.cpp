#include "vec_group.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "ved_object_io.h"
#include "vec_object_geometry.h"
#include "vec_ellipse.h"

#include <array>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <span>

namespace {

void writePoint(VEDBinaryWriter& writer, TDMatPoint point)
{
    writer.WriteDouble(point.x);
    writer.WriteDouble(point.y);
}

TDMatPoint readPoint(VEDBinaryReader& reader)
{
    TDMatPoint point;
    point.x = reader.ReadDouble();
    point.y = reader.ReadDouble();
    return point;
}

void writeRect(VEDBinaryWriter& writer, TDMatRect rect)
{
    writePoint(writer, rect.P1);
    writePoint(writer, rect.P2);
    writePoint(writer, rect.P3);
    writePoint(writer, rect.P4);
}

TDMatRect readRect(VEDBinaryReader& reader)
{
    TDMatRect rect;
    rect.P1 = readPoint(reader);
    rect.P2 = readPoint(reader);
    rect.P3 = readPoint(reader);
    rect.P4 = readPoint(reader);
    return rect;
}

} // namespace

TDVecGroup::TDVecGroup()
    : rotateAngle_(0.0),
      xScale_(1.0),
      yScale_(1.0),
      topLeftBoundary_{0.0, 0.0},
      bottomRightBoundary_{0.0, 0.0},
      originPoint_{0.0, 0.0},
      xLength_(0.0),
      yLength_(0.0),
      objectInGroupCount_(0),
      resolveLock_(false),
      oldFrame_{} {
    SetType(VECOBJ_GRP);
}

std::uint32_t TDVecGroup::StreamFourCC()
{
    return VEDMakeFourCC('g', 'r', 'u', 'p');
}

std::uint32_t TDVecGroup::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecGroup> TDVecGroup::ReadFrom(VEDBinaryReader& reader)
{
    auto group = std::make_unique<TDVecGroup>();
    group->ReadObjectStateFrom(reader);
    group->rotateAngle_ = reader.ReadDouble();
    group->xScale_ = reader.ReadDouble();
    group->yScale_ = reader.ReadDouble();
    group->topLeftBoundary_ = readPoint(reader);
    group->bottomRightBoundary_ = readPoint(reader);
    group->originPoint_ = readPoint(reader);
    group->xLength_ = reader.ReadDouble();
    group->yLength_ = reader.ReadDouble();
    group->resolveLock_ = reader.ReadBool();
    group->oldFrame_ = readRect(reader);
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return nullptr;
    }

    group->objectsInGroup_.clear();
    group->objectsInGroup_.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        const std::uint32_t typeFourCC = reader.ReadFourCC();
        const std::uint32_t payloadSize = reader.ReadUInt32();
        const std::span<const std::byte> payload = reader.ReadBytes(payloadSize);
        if(!reader.Ok()) {
            return nullptr;
        }

        VEDObjectReadResult objectResult = LoadVecObjectPayload(typeFourCC, payload);
        if(!objectResult.Ok()) {
            reader.Fail(objectResult.error);
            return nullptr;
        }
        objectResult.object->SetParentModel(nullptr);
        group->objectsInGroup_.push_back(std::move(objectResult.object));
    }
    group->objectInGroupCount_ = static_cast<long>(group->objectsInGroup_.size());
    return reader.Ok() ? std::move(group) : nullptr;
}

void TDVecGroup::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteDouble(rotateAngle_);
    writer.WriteDouble(xScale_);
    writer.WriteDouble(yScale_);
    writePoint(writer, topLeftBoundary_);
    writePoint(writer, bottomRightBoundary_);
    writePoint(writer, originPoint_);
    writer.WriteDouble(xLength_);
    writer.WriteDouble(yLength_);
    writer.WriteBool(resolveLock_);
    writeRect(writer, oldFrame_);
    writer.WriteInt32(static_cast<std::int32_t>(objectsInGroup_.size()));
    for(const auto& object : objectsInGroup_) {
        const VEDObjectWriteResult objectResult = SaveVecObjectPayload(*object);
        writer.WriteFourCC(objectResult.typeFourCC);
        writer.WriteUInt32(static_cast<std::uint32_t>(objectResult.payload.size()));
        writer.WriteBytes(objectResult.payload);
    }
}

TDVecGroup::TDVecGroup(const TDVecGroup& group)
    : TDVecObject(group),
      rotateAngle_(group.rotateAngle_),
      xScale_(group.xScale_),
      yScale_(group.yScale_),
      topLeftBoundary_(group.topLeftBoundary_),
      bottomRightBoundary_(group.bottomRightBoundary_),
      originPoint_(group.originPoint_),
      xLength_(group.xLength_),
      yLength_(group.yLength_),
      objectInGroupCount_(group.objectInGroupCount_),
      resolveLock_(group.resolveLock_),
      oldFrame_(group.oldFrame_) {
    objectsInGroup_.reserve(group.objectsInGroup_.size());
    for (const auto& object : group.objectsInGroup_) {
        if (object) {
            objectsInGroup_.push_back(object->Clone());
        }
    }
}

TDVecGroup& TDVecGroup::operator=(const TDVecGroup& group) {
    if (this == &group) {
        return *this;
    }

    SetSelect(group.GetSelect());
    SetColor(group.GetColor());
    SetLockResize(group.GetLockResize());
    SetLockPosition(group.GetLockPosition());
    rotateAngle_ = group.rotateAngle_;
    xScale_ = group.xScale_;
    yScale_ = group.yScale_;
    topLeftBoundary_ = group.topLeftBoundary_;
    bottomRightBoundary_ = group.bottomRightBoundary_;
    originPoint_ = group.originPoint_;
    xLength_ = group.xLength_;
    yLength_ = group.yLength_;
    objectInGroupCount_ = group.objectInGroupCount_;
    resolveLock_ = group.resolveLock_;
    oldFrame_ = group.oldFrame_;
    objectsInGroup_.clear();
    objectsInGroup_.reserve(group.objectsInGroup_.size());
    for (const auto& object : group.objectsInGroup_) {
        if (object) {
            objectsInGroup_.push_back(object->Clone());
        }
    }
    return *this;
}

void TDVecGroup::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE) {
        return;
    }

    for (const auto& object : objectsInGroup_) {
        if (object) {
            std::unique_ptr<TDVecObject> clone = object->Clone();
            clone->TransformToOrigin(originPoint_);
            clone->ToScale(originPoint_, xScale_, yScale_);
            clone->Rotate(originPoint_, rotateAngle_);
            clone->Draw(pGE, bOutLine);
        }
    }
}

void TDVecGroup::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE) {
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

TDMatRect TDVecGroup::GetFrame() const {
    const TDMatRect diagonalFrame{
        {-xLength_, -yLength_},
        {xLength_, -yLength_},
        {xLength_, yLength_},
        {-xLength_, yLength_}
    };
    const double angle = rotateAngle_;
    return {
        {
            (cosD(angle) * diagonalFrame.P1.x - sinD(angle) * diagonalFrame.P1.y) + originPoint_.x,
            (sinD(angle) * diagonalFrame.P1.x + cosD(angle) * diagonalFrame.P1.y) + originPoint_.y
        },
        {
            (cosD(angle) * diagonalFrame.P2.x - sinD(angle) * diagonalFrame.P2.y) + originPoint_.x,
            (sinD(angle) * diagonalFrame.P2.x + cosD(angle) * diagonalFrame.P2.y) + originPoint_.y
        },
        {
            (cosD(angle) * diagonalFrame.P3.x - sinD(angle) * diagonalFrame.P3.y) + originPoint_.x,
            (sinD(angle) * diagonalFrame.P3.x + cosD(angle) * diagonalFrame.P3.y) + originPoint_.y
        },
        {
            (cosD(angle) * diagonalFrame.P4.x - sinD(angle) * diagonalFrame.P4.y) + originPoint_.x,
            (sinD(angle) * diagonalFrame.P4.x + cosD(angle) * diagonalFrame.P4.y) + originPoint_.y
        }
    };
}

TDMatPoint TDVecGroup::GetMidpoint() const {
    return originPoint_;
}

std::unique_ptr<TDVecObject> TDVecGroup::Clone() const {
    return std::make_unique<TDVecGroup>(*this);
}

TDVecLineHitResult TDVecGroup::HitTest(TDMatPoint point, double tolerance) const {
    for (auto it = objectsInGroup_.rbegin(); it != objectsInGroup_.rend(); ++it) {
        if (*it) {
            std::unique_ptr<TDVecObject> clone = (*it)->Clone();
            clone->TransformToOrigin(originPoint_);
            clone->ToScale(originPoint_, xScale_, yScale_);
            clone->Rotate(originPoint_, rotateAngle_);
            const TDVecLineHitResult hit = clone->HitTest(point, tolerance);
            if (hit.IsHit()) {
                return hit;
            }
        }
    }
    return {};
}

TDVecLineHitResult TDVecGroup::HitTestNode(TDMatPoint point, double tolerance) const {
    for (auto it = objectsInGroup_.rbegin(); it != objectsInGroup_.rend(); ++it) {
        if (*it) {
            std::unique_ptr<TDVecObject> clone = (*it)->Clone();
            clone->TransformToOrigin(originPoint_);
            clone->ToScale(originPoint_, xScale_, yScale_);
            clone->Rotate(originPoint_, rotateAngle_);
            const TDVecLineHitResult hit = clone->HitTestNode(point, tolerance);
            if (hit.IsHit()) {
                return hit;
            }
        }
    }
    return {};
}

TDVecLineHitResult TDVecGroup::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    for (auto it = objectsInGroup_.rbegin(); it != objectsInGroup_.rend(); ++it) {
        if (*it) {
            std::unique_ptr<TDVecObject> clone = (*it)->Clone();
            clone->TransformToOrigin(originPoint_);
            clone->ToScale(originPoint_, xScale_, yScale_);
            clone->Rotate(originPoint_, rotateAngle_);
            const TDVecLineHitResult hit = clone->HitTest(pGE, point, tolerancePixels);
            if (hit.IsHit()) {
                return hit;
            }
        }
    }
    return {};
}

TDVecLineHitResult TDVecGroup::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    for (auto it = objectsInGroup_.rbegin(); it != objectsInGroup_.rend(); ++it) {
        if (*it) {
            std::unique_ptr<TDVecObject> clone = (*it)->Clone();
            clone->TransformToOrigin(originPoint_);
            clone->ToScale(originPoint_, xScale_, yScale_);
            clone->Rotate(originPoint_, rotateAngle_);
            const TDVecLineHitResult hit = clone->HitTestNode(pGE, point, tolerancePixels);
            if (hit.IsHit()) {
                return hit;
            }
        }
    }
    return {};
}

long TDVecGroup::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    return PointOnFrameNode(pGE, X, Y, Toleranz);
}

long TDVecGroup::PointOnNode(TDMatPoint point, double tolerance) const {
    return PointOnFrameNode(point, tolerance);
}

bool TDVecGroup::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    if (GetLockResize()) {
        return false;
    }
    if (!MatBelike2Double(rotateAngle_, 0.0, 4)) {
        return false;
    }

    return MoveFrameNode(iNode, X, Y, MatCPoint);
}

bool TDVecGroup::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    originPoint_.x += dx;
    originPoint_.y += dy;
    return true;
}

bool TDVecGroup::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (MatBelike2Double(xScale, 0.0, 4) || MatBelike2Double(yScale, 0.0, 4)) {
        return false;
    }
    if (GetLockResize()) {
        return false;
    }

    oldFrame_ = GetFrame();
    const TDMatPoint oldOriginPoint = originPoint_;
    if (xScale == 1.0 && yScale == 1.0) {
        return false;
    }

    std::vector<std::unique_ptr<TDVecObject>> objectsInGroupCloned;
    objectsInGroupCloned.reserve(objectsInGroup_.size());
    for (const auto& object : objectsInGroup_) {
        if (object) {
            objectsInGroupCloned.push_back(object->Clone());
        }
    }

    for (const auto& object : objectsInGroup_) {
        if (object) {
            object->TransformToOrigin(originPoint_);
            object->ToScale(originPoint_, xScale_, yScale_);
        }
    }
    for (const auto& object : objectsInGroup_) {
        if (object) {
            object->ToScale(MatPoint, xScale, yScale);
        }
    }
    FirstInitialize();

    for (const auto& object : objectsInGroup_) {
        if (object) {
            object->MoveBy(oldOriginPoint.x - originPoint_.x, oldOriginPoint.y - originPoint_.y);
        }
    }

    objectsInGroup_.clear();
    objectsInGroup_.reserve(objectsInGroupCloned.size());
    for (const auto& object : objectsInGroupCloned) {
        if (object) {
            objectsInGroup_.push_back(object->Clone());
        }
    }
    xScale_ = xScale_ * xScale;
    yScale_ = yScale_ * yScale;
    return true;
}

bool TDVecGroup::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    std::vector<std::unique_ptr<TDVecObject>> objectsInGroupCopy;
    objectsInGroupCopy.reserve(objectsInGroup_.size());
    for (long i = 0; i < CountObjectsInGroup(); i++) {
        std::unique_ptr<TDVecObject> object = GetCopyFromGroup(i);
        if (object) {
            object->Rotate(MatPoint, nAngle);
            objectsInGroupCopy.push_back(std::move(object));
        }
    }

    objectsInGroup_.clear();
    for (const auto& object : objectsInGroupCopy) {
        if (object) {
            std::unique_ptr<TDVecObject> clone = object->Clone();
            clone->SetSelect(false);
            AppendInGroup(clone.release());
        }
    }

    rotateAngle_ = 0.0;
    xScale_ = 1.0;
    yScale_ = 1.0;
    FirstInitialize();
    return true;
}

void TDVecGroup::TransformToPoint(TDMatPoint MatPoint) {
    originPoint_.x -= MatPoint.x;
    originPoint_.y -= MatPoint.y;
}

void TDVecGroup::TransformToOrigin(TDMatPoint MatPoint) {
    originPoint_.x += MatPoint.x;
    originPoint_.y += MatPoint.y;
}

void TDVecGroup::AppendInGroup(TDVecObject* pObject) {
    if (!pObject) {
        return;
    }

    pObject->SetLockResize(false);
    pObject->SetLockPosition(false);
    pObject->SetSelect(false);
    pObject->SetParentModel(nullptr);
    objectsInGroup_.emplace_back(pObject);
}

TDVecObject* TDVecGroup::GetFromGroup(long iObject) const {
    if (iObject < 0 || iObject >= CountObjectsInGroup()) {
        return nullptr;
    }
    TDVecObject* object = objectsInGroup_[static_cast<std::size_t>(iObject)].get();
    if (!object) {
        return nullptr;
    }
    object->TransformToOrigin(originPoint_);
    object->ToScale(originPoint_, xScale_, yScale_);
    object->Rotate(originPoint_, rotateAngle_);
    object->Initialize();
    return object;
}

std::unique_ptr<TDVecObject> TDVecGroup::GetCopyFromGroup(long iObject) const {
    if (iObject < 0 || iObject >= CountObjectsInGroup()) {
        return nullptr;
    }
    TDVecObject* object = objectsInGroup_[static_cast<std::size_t>(iObject)].get();
    if (!object) {
        return nullptr;
    }
    std::unique_ptr<TDVecObject> clone = object->Clone();
    clone->TransformToOrigin(originPoint_);
    clone->ToScale(originPoint_, xScale_, yScale_);
    clone->Rotate(originPoint_, rotateAngle_);
    return clone;
}

void TDVecGroup::ForEachStoredObject(const std::function<void(TDVecObject&)>& visitor) {
    if (!visitor) {
        return;
    }
    for (const auto& object : objectsInGroup_) {
        if (object) {
            visitor(*object);
        }
    }
}

long TDVecGroup::CountObjectsInGroup() const {
    return static_cast<long>(objectsInGroup_.size());
}

void TDVecGroup::FirstInitialize() {
    CalculateBoundary();
    originPoint_.x = (topLeftBoundary_.x + bottomRightBoundary_.x) / 2.0;
    originPoint_.y = (topLeftBoundary_.y + bottomRightBoundary_.y) / 2.0;
    xLength_ = (bottomRightBoundary_.x - topLeftBoundary_.x) / 2.0;
    yLength_ = (bottomRightBoundary_.y - topLeftBoundary_.y) / 2.0;
    objectInGroupCount_ = static_cast<long>(objectsInGroup_.size());

    for (const auto& object : objectsInGroup_) {
        if (object) {
            object->TransformToPoint(originPoint_);
        }
    }
}

void TDVecGroup::SetResolveLock(bool bResolveLock) {
    resolveLock_ = bResolveLock;
}

bool TDVecGroup::GetResolveLock() const {
    return resolveLock_;
}

void TDVecGroup::CalculateBoundary() const {
    if (objectsInGroup_.empty()) {
        topLeftBoundary_ = {0.0, 0.0};
        bottomRightBoundary_ = {0.0, 0.0};
        return;
    }

    bool hasPoint = false;
    double xLeft = 0.0;
    double yTop = 0.0;
    double xRight = 0.0;
    double yBottom = 0.0;
    for (const auto& object : objectsInGroup_) {
        if (!object) {
            continue;
        }

        TDMatRect frameRect = object->GetFrame();
        if (object->GetType() == VECOBJ_ELL) {
            auto* ellipse = dynamic_cast<TDVecEllipse*>(object.get());
            if (ellipse && ellipse->GetAngle() == 0.0) {
                frameRect = frameMidEdges(frameRect);
            }
        }
        const std::array points{frameRect.P1, frameRect.P2, frameRect.P3, frameRect.P4};
        for (const TDMatPoint& point : points) {
            if (!hasPoint) {
                xLeft = xRight = point.x;
                yTop = yBottom = point.y;
                hasPoint = true;
            } else {
                xLeft = std::min(xLeft, point.x);
                yTop = std::min(yTop, point.y);
                xRight = std::max(xRight, point.x);
                yBottom = std::max(yBottom, point.y);
            }
        }
    }

    topLeftBoundary_ = {xLeft, yTop};
    bottomRightBoundary_ = {xRight, yBottom};
}
