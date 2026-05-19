#include "vec_model.h"

#include "vec_group.h"

#include <algorithm>

namespace {

struct FrameExtents {
    double left = 0.0;
    double right = 0.0;
    double top = 0.0;
    double bottom = 0.0;
};

bool isPointInBounds(TDMatPoint point, double left, double top, double right, double bottom) {
    return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
}

bool isFrameInBounds(TDMatRect frame, double left, double top, double right, double bottom) {
    return isPointInBounds(frame.P1, left, top, right, bottom) &&
           isPointInBounds(frame.P2, left, top, right, bottom) &&
           isPointInBounds(frame.P3, left, top, right, bottom) &&
           isPointInBounds(frame.P4, left, top, right, bottom);
}

FrameExtents frameExtents(TDMatRect frame) {
    FrameExtents extents;
    extents.left = std::min({frame.P1.x, frame.P2.x, frame.P3.x, frame.P4.x});
    extents.right = std::max({frame.P1.x, frame.P2.x, frame.P3.x, frame.P4.x});
    extents.top = std::min({frame.P1.y, frame.P2.y, frame.P3.y, frame.P4.y});
    extents.bottom = std::max({frame.P1.y, frame.P2.y, frame.P3.y, frame.P4.y});
    return extents;
}

} // namespace

bool TDVecModelHitResult::IsHit() const {
    return objectIndex >= 0 && object != nullptr && lineHit.IsHit();
}

bool TDVecModelHitResult::IsNodeHit() const {
    return IsHit() && lineHit.IsNodeHit();
}

TDVecModel::TDVecModel()
    : topLeftArea_{0.0, 0.0},
      bottomRightArea_{210000.0, 296985.0},
      defaultColor_(0x00000000),
      changed_(false),
      revision_(0) {
}

TDVecModel::~TDVecModel() = default;

TDMatPoint TDVecModel::GetTopLeftArea() const {
    return topLeftArea_;
}

TDMatPoint TDVecModel::GetBottomRightArea() const {
    return bottomRightArea_;
}

bool TDVecModel::SetTopLeftArea(TDMatPoint point) {
    if (topLeftArea_.x != point.x || topLeftArea_.y != point.y) {
        MarkChanged();
    }
    topLeftArea_ = point;
    return true;
}

bool TDVecModel::SetBottomRightArea(TDMatPoint point) {
    if (bottomRightArea_.x != point.x || bottomRightArea_.y != point.y) {
        MarkChanged();
    }
    bottomRightArea_ = point;
    return true;
}

void TDVecModel::SetDefaultColor(TDRgbColor color) {
    if (defaultColor_ != color) {
        MarkChanged();
    }
    defaultColor_ = color;
}

TDRgbColor TDVecModel::GetDefaultColor() const {
    return defaultColor_;
}

bool TDVecModel::IsChanged() const {
    return changed_;
}

void TDVecModel::SetChanged(bool changed) {
    changed_ = changed;
}

void TDVecModel::MarkChanged() {
    changed_ = true;
    ++revision_;
}

std::uint64_t TDVecModel::Revision() const {
    return revision_;
}

TDVecModelSnapshot TDVecModel::CreateSnapshot() const {
    TDVecModelSnapshot snapshot;
    snapshot.topLeftArea = topLeftArea_;
    snapshot.bottomRightArea = bottomRightArea_;
    snapshot.defaultColor = defaultColor_;
    snapshot.objects.reserve(objects_.size());
    for (const auto& object : objects_) {
        if (object) {
            snapshot.objects.push_back(object->Clone());
        }
    }
    return snapshot;
}

void TDVecModel::RestoreSnapshot(const TDVecModelSnapshot& snapshot, bool changed) {
    objects_.clear();
    objects_.reserve(snapshot.objects.size());
    for (const auto& object : snapshot.objects) {
        if (!object) {
            continue;
        }
        auto clone = object->Clone();
        clone->SetParentModel(this);
        objects_.push_back(std::move(clone));
    }
    topLeftArea_ = snapshot.topLeftArea;
    bottomRightArea_ = snapshot.bottomRightArea;
    defaultColor_ = snapshot.defaultColor;
    changed_ = changed;
    ++revision_;
}

TDVecObject* TDVecModel::AppendObject(TDVecObject* pObject) {
    if (!pObject) {
        return nullptr;
    }

    pObject->SetParentModel(this);
    objects_.emplace_back(pObject);
    MarkChanged();
    return pObject;
}

TDVecObject* TDVecModel::GetObject(int iObject) const {
    if (iObject < 0 || iObject >= static_cast<int>(objects_.size())) {
        return nullptr;
    }

    return objects_[static_cast<std::size_t>(iObject)].get();
}

int TDVecModel::CountObjects() const {
    return static_cast<int>(objects_.size());
}

TDVecModelHitResult TDVecModel::FindObjectAt(TDMatPoint point, double tolerance) const {
    for (int index = CountObjects() - 1; index >= 0; --index) {
        TDVecObject* object = GetObject(index);
        if (!object) {
            continue;
        }

        const TDVecLineHitResult hit = object->HitTest(point, tolerance);
        if (hit.IsHit()) {
            return {index, object, hit};
        }
    }

    return {};
}

TDVecModelHitResult TDVecModel::FindObjectAt(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    if (!pGE) {
        return {};
    }

    for (int index = CountObjects() - 1; index >= 0; --index) {
        TDVecObject* object = GetObject(index);
        if (!object) {
            continue;
        }

        const TDVecLineHitResult hit = object->HitTest(pGE, point, tolerancePixels);
        if (hit.IsHit()) {
            return {index, object, hit};
        }
    }

    return {};
}

TDVecModelHitResult TDVecModel::FindNodeAt(TDMatPoint point, double tolerance) const {
    for (int index = CountObjects() - 1; index >= 0; --index) {
        TDVecObject* object = GetObject(index);
        if (!object) {
            continue;
        }

        const TDVecLineHitResult hit = object->HitTestNode(point, tolerance);
        if (hit.IsHit()) {
            return {index, object, hit};
        }
    }

    return {};
}

TDVecModelHitResult TDVecModel::FindNodeAt(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    if (!pGE) {
        return {};
    }

    for (int index = CountObjects() - 1; index >= 0; --index) {
        TDVecObject* object = GetObject(index);
        if (!object) {
            continue;
        }

        const TDVecLineHitResult hit = object->HitTestNode(pGE, point, tolerancePixels);
        if (hit.IsHit()) {
            return {index, object, hit};
        }
    }

    return {};
}

void TDVecModel::DeselectAll() {
    for (const auto& object : objects_) {
        object->SetSelect(false);
    }
}

bool TDVecModel::SelectObject(int iObject) {
    TDVecObject* object = GetObject(iObject);
    if (!object) {
        return false;
    }

    object->SetSelect(true);
    return true;
}

bool TDVecModel::SelectOnlyObject(int iObject) {
    TDVecObject* object = GetObject(iObject);
    if (!object) {
        return false;
    }

    DeselectAll();
    object->SetSelect(true);
    return true;
}

int TDVecModel::SelectObjectsInArea(TDMatPoint firstPoint, TDMatPoint secondPoint, bool addToSelection) {
    const double left = std::min(firstPoint.x, secondPoint.x);
    const double right = std::max(firstPoint.x, secondPoint.x);
    const double top = std::min(firstPoint.y, secondPoint.y);
    const double bottom = std::max(firstPoint.y, secondPoint.y);

    if (!addToSelection) {
        DeselectAll();
    }

    int selectedCount = 0;
    for (const auto& object : objects_) {
        if (object && isFrameInBounds(object->GetFrame(), left, top, right, bottom)) {
            object->SetSelect(true);
            ++selectedCount;
        }
    }
    return selectedCount;
}

TDVecObject* TDVecModel::GetSelectedObject() const {
    const int index = GetSelectedObjectIndex();
    return GetObject(index);
}

int TDVecModel::GetSelectedObjectIndex() const {
    for (int index = 0; index < CountObjects(); ++index) {
        const TDVecObject* object = GetObject(index);
        if (object && object->GetSelect()) {
            return index;
        }
    }

    return -1;
}

int TDVecModel::CountSelectedObjects() const {
    int selectedCount = 0;
    for (const auto& object : objects_) {
        if (object->GetSelect()) {
            ++selectedCount;
        }
    }
    return selectedCount;
}

bool TDVecModel::IsObjectSelected(int iObject) const {
    const TDVecObject* object = GetObject(iObject);
    return object && object->GetSelect();
}

std::vector<TDVecObject*> TDVecModel::GetSelectedObjects() const {
    std::vector<TDVecObject*> selectedObjects;
    for (const auto& object : objects_) {
        if (object && object->GetSelect()) {
            selectedObjects.push_back(object.get());
        }
    }
    return selectedObjects;
}

int TDVecModel::MoveSelectedObjects(double dx, double dy) {
    int movedCount = 0;
    for (const auto& object : objects_) {
        if (object && object->GetSelect()) {
            if (object->MoveBy(dx, dy)) {
                ++movedCount;
            }
        }
    }
    if (movedCount > 0) {
        MarkChanged();
    }
    return movedCount;
}

bool TDVecModel::SetLockResizeForSelectedObjects(bool value) {
    bool changedAny = false;
    for (const auto& object : objects_) {
        if (object && object->GetSelect()) {
            if (object->GetLockResize() != value) {
                object->SetLockResize(value);
                object->Initialize();
                changedAny = true;
            }
        }
    }
    if (changedAny) {
        MarkChanged();
    }
    return changedAny;
}

bool TDVecModel::SetLockPositionForSelectedObjects(bool value) {
    bool changedAny = false;
    for (const auto& object : objects_) {
        if (object && object->GetSelect()) {
            if (object->GetLockPosition() != value) {
                object->SetLockPosition(value);
                object->Initialize();
                changedAny = true;
            }
        }
    }
    if (changedAny) {
        MarkChanged();
    }
    return changedAny;
}

unsigned long TDVecModel::GetLockResizeForSelectedObjects() const {
    unsigned long lockedCount = 0;
    for (const auto& object : objects_) {
        if (object && object->GetSelect() && object->GetLockResize()) {
            ++lockedCount;
        }
    }
    return lockedCount;
}

unsigned long TDVecModel::GetLockPositionForSelectedObjects() const {
    unsigned long lockedCount = 0;
    for (const auto& object : objects_) {
        if (object && object->GetSelect() && object->GetLockPosition()) {
            ++lockedCount;
        }
    }
    return lockedCount;
}

int TDVecModel::AlignSelectedObjects(TDVecAlignMode mode) {
    const std::vector<TDVecObject*> selectedObjects = GetSelectedObjects();
    if (selectedObjects.size() <= 1) {
        return 0;
    }

    FrameExtents target = frameExtents(selectedObjects.front()->GetFrame());
    for (std::size_t index = 1; index < selectedObjects.size(); ++index) {
        const FrameExtents extents = frameExtents(selectedObjects[index]->GetFrame());
        target.left = std::min(target.left, extents.left);
        target.right = std::max(target.right, extents.right);
        target.top = std::min(target.top, extents.top);
        target.bottom = std::max(target.bottom, extents.bottom);
    }

    const double targetCenterX = (target.left + target.right) / 2.0;
    const double targetCenterY = (target.top + target.bottom) / 2.0;
    int movedCount = 0;

    for (TDVecObject* object : selectedObjects) {
        if (!object) {
            continue;
        }

        const FrameExtents extents = frameExtents(object->GetFrame());
        double dx = 0.0;
        double dy = 0.0;

        switch (mode) {
        case TDVecAlignMode::Left:
            dx = target.left - extents.left;
            break;
        case TDVecAlignMode::HorizontalCenter:
            dx = targetCenterX - ((extents.left + extents.right) / 2.0);
            break;
        case TDVecAlignMode::Right:
            dx = target.right - extents.right;
            break;
        case TDVecAlignMode::Top:
            dy = target.top - extents.top;
            break;
        case TDVecAlignMode::VerticalMiddle:
            dy = targetCenterY - ((extents.top + extents.bottom) / 2.0);
            break;
        case TDVecAlignMode::Bottom:
            dy = target.bottom - extents.bottom;
            break;
        }

        if ((dx != 0.0 || dy != 0.0) && object->MoveBy(dx, dy)) {
            ++movedCount;
        }
    }

    if (movedCount > 0) {
        MarkChanged();
    }
    return movedCount;
}

bool TDVecModel::MakeGroup() {
    std::vector<int> selectedIndexes;
    for (int index = 0; index < CountObjects(); ++index) {
        TDVecObject* object = GetObject(index);
        if (object && object->GetSelect() && !object->GetLockObject()) {
            selectedIndexes.push_back(index);
        }
    }

    if (selectedIndexes.size() <= 1) {
        return false;
    }

    auto groupObject = std::make_unique<TDVecGroup>();
    for (int index : selectedIndexes) {
        TDVecObject* object = GetObject(index);
        if (!object || !object->GetSelect()) {
            continue;
        }

        std::unique_ptr<TDVecObject> clone = object->Clone();
        clone->SetSelect(false);
        groupObject->AppendInGroup(clone.release());
    }

    for (auto it = selectedIndexes.rbegin(); it != selectedIndexes.rend(); ++it) {
        DeleteObject(*it);
    }

    groupObject->FirstInitialize();
    groupObject->SetSelect(true);
    AppendObject(groupObject.release());
    MarkChanged();
    return true;
}

bool TDVecModel::ResolveGroup(int iObjGroup) {
    TDVecObject* object = GetObject(iObjGroup);
    auto* groupObject = dynamic_cast<TDVecGroup*>(object);
    if (!groupObject || groupObject->GetResolveLock() || groupObject->GetLockObject()) {
        return false;
    }

    const long count = groupObject->CountObjectsInGroup();
    for (long index = 0; index < count; ++index) {
        std::unique_ptr<TDVecObject> clone = groupObject->GetCopyFromGroup(index);
        if (!clone) {
            continue;
        }
        clone->SetSelect(true);
        AppendObject(clone.release());
    }

    DeleteObject(iObjGroup);
    MarkChanged();
    return true;
}

bool TDVecModel::ResolveAllSelectedGroups() {
    std::vector<int> selectedGroupIndexes;
    for (int index = 0; index < CountObjects(); ++index) {
        TDVecObject* object = GetObject(index);
        auto* groupObject = dynamic_cast<TDVecGroup*>(object);
        if (groupObject && groupObject->GetSelect() && !groupObject->GetResolveLock() && !groupObject->GetLockObject()) {
            selectedGroupIndexes.push_back(index);
        }
    }

    if (selectedGroupIndexes.empty()) {
        return false;
    }

    for (int groupIndex : selectedGroupIndexes) {
        auto* groupObject = dynamic_cast<TDVecGroup*>(GetObject(groupIndex));
        if (!groupObject) {
            continue;
        }

        const long count = groupObject->CountObjectsInGroup();
        for (long objectIndex = 0; objectIndex < count; ++objectIndex) {
            std::unique_ptr<TDVecObject> clone = groupObject->GetCopyFromGroup(objectIndex);
            if (!clone) {
                continue;
            }
            clone->SetSelect(true);
            AppendObject(clone.release());
        }
    }

    for (auto it = selectedGroupIndexes.rbegin(); it != selectedGroupIndexes.rend(); ++it) {
        DeleteObject(*it);
    }
    MarkChanged();
    return true;
}

bool TDVecModel::DeleteObject(int iObject) {
    if (iObject < 0 || iObject >= CountObjects()) {
        return false;
    }

    const TDVecObject* object = GetObject(iObject);
    if (!object || object->GetLockObject()) {
        return false;
    }

    objects_.erase(objects_.begin() + iObject);
    MarkChanged();
    return true;
}

int TDVecModel::DeleteSelectedObjects() {
    int deletedCount = 0;
    for (int index = CountObjects() - 1; index >= 0; --index) {
        const TDVecObject* object = GetObject(index);
        if (object && object->GetSelect() && !object->GetLockObject()) {
            DeleteObject(index);
            ++deletedCount;
        }
    }
    return deletedCount;
}

bool TDVecModel::ClearObjects() {
    if (!objects_.empty()) {
        MarkChanged();
    }
    objects_.clear();
    return true;
}
