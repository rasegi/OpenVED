#pragma once

#include "vec_document_settings.h"
#include "vec_object.h"

#include <cstdint>
#include <memory>
#include <vector>

struct TDVecModelHitResult {
    int objectIndex = -1;
    TDVecObject* object = nullptr;
    TDVecLineHitResult lineHit;

    bool IsHit() const;
    bool IsNodeHit() const;
};

struct TDVecModelSnapshot {
    std::vector<std::unique_ptr<TDVecObject>> objects;
    TDMatPoint topLeftArea;
    TDMatPoint bottomRightArea;
    TDRgbColor defaultColor;
    TDVecDocumentSettings documentSettings;
};

enum class TDVecAlignMode {
    Left,
    HorizontalCenter,
    Right,
    Top,
    VerticalMiddle,
    Bottom
};

class TDVecModel {
public:
    TDVecModel();
    ~TDVecModel();

    TDMatPoint GetTopLeftArea() const;
    TDMatPoint GetBottomRightArea() const;
    bool SetTopLeftArea(TDMatPoint point);
    bool SetBottomRightArea(TDMatPoint point);
    void SetDefaultColor(TDRgbColor color);
    TDRgbColor GetDefaultColor() const;
    bool IsChanged() const;
    void SetChanged(bool changed);
    void MarkChanged();
    std::uint64_t Revision() const;
    const TDVecDocumentSettings& DocumentSettings() const;
    const TDVecUnitSettings& UnitSettings() const;
    const TDVecGridSettings& GridSettings() const;
    const TDVecPageSettings& PageSettings() const;
    void SetDocumentSettings(const TDVecDocumentSettings& settings);

    TDVecModelSnapshot CreateSnapshot() const;
    void RestoreSnapshot(const TDVecModelSnapshot& snapshot, bool changed);

    TDVecObject* AppendObject(TDVecObject* pObject);
    TDVecObject* GetObject(int iObject) const;
    int CountObjects() const;
    TDVecModelHitResult FindObjectAt(TDMatPoint point, double tolerance) const;
    TDVecModelHitResult FindObjectAt(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const;
    TDVecModelHitResult FindNodeAt(TDMatPoint point, double tolerance) const;
    TDVecModelHitResult FindNodeAt(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const;
    void DeselectAll();
    bool SelectObject(int iObject);
    bool SelectOnlyObject(int iObject);
    int SelectObjectsInArea(TDMatPoint firstPoint, TDMatPoint secondPoint, bool addToSelection);
    TDVecObject* GetSelectedObject() const;
    int GetSelectedObjectIndex() const;
    int CountSelectedObjects() const;
    bool IsObjectSelected(int iObject) const;
    std::vector<TDVecObject*> GetSelectedObjects() const;
    int MoveSelectedObjects(double dx, double dy);
    bool SetLockResizeForSelectedObjects(bool value);
    bool SetLockPositionForSelectedObjects(bool value);
    unsigned long GetLockResizeForSelectedObjects() const;
    unsigned long GetLockPositionForSelectedObjects() const;
    int AlignSelectedObjects(TDVecAlignMode mode);
    bool MakeGroup();
    bool ResolveGroup(int iObjGroup);
    bool ResolveAllSelectedGroups();
    bool DeleteObject(int iObject);
    int DeleteSelectedObjects();
    bool ClearObjects();

private:
    std::vector<std::unique_ptr<TDVecObject>> objects_;
    TDMatPoint topLeftArea_;
    TDMatPoint bottomRightArea_;
    TDRgbColor defaultColor_;
    TDVecDocumentSettings documentSettings_;
    bool changed_;
    std::uint64_t revision_;
};
