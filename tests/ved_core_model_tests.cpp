#include "vec_group.h"
#include "vec_edit_cad.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vop_manager.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace {

struct TestFrameExtents {
    double left = 0.0;
    double right = 0.0;
    double top = 0.0;
    double bottom = 0.0;
};

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

int expect(bool condition, const std::string& message) {
    return condition ? 0 : fail(message);
}

TestFrameExtents extentsOf(const TDVecObject* object) {
    const TDMatRect frame = object->GetFrame();
    return {
        std::min(std::min(frame.P1.x, frame.P2.x), std::min(frame.P3.x, frame.P4.x)),
        std::max(std::max(frame.P1.x, frame.P2.x), std::max(frame.P3.x, frame.P4.x)),
        std::min(std::min(frame.P1.y, frame.P2.y), std::min(frame.P3.y, frame.P4.y)),
        std::max(std::max(frame.P1.y, frame.P2.y), std::max(frame.P3.y, frame.P4.y))
    };
}

int expectEqual(double actual, double expected, const std::string& message) {
    return actual == expected ? 0 : fail(message);
}

} // namespace

int main() {
    TDVecModel model;
    if (int result = expect(!model.IsChanged(), "new model started dirty")) {
        return result;
    }
    auto* firstLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 0.0, 100.0, 0.0)));
    auto* secondLine = dynamic_cast<TDVecLine*>(model.AppendObject(new TDVecLine(0.0, 10.0, 100.0, 10.0)));

    if (int result = expect(firstLine != nullptr && secondLine != nullptr, "append line failed")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 2, "object count after append failed")) {
        return result;
    }
    if (int result = expect(model.IsChanged(), "append did not dirty model")) {
        return result;
    }
    model.SetChanged(false);
    if (int result = expect(!model.IsChanged(), "SetChanged(false) did not clear dirty flag")) {
        return result;
    }

    TDVecModelHitResult bodyHit = model.FindObjectAt({25.0, 1.0}, 2.0);
    if (int result = expect(bodyHit.IsHit(), "body hit failed")) {
        return result;
    }
    if (int result = expect(bodyHit.objectIndex == 0, "body hit index failed")) {
        return result;
    }
    if (int result = expect(bodyHit.lineHit.kind == TDVecLineHitKind::Body, "body hit kind failed")) {
        return result;
    }

    TDVecModelHitResult topHit = model.FindObjectAt({50.0, 5.0}, 6.0);
    if (int result = expect(topHit.IsHit(), "topmost hit failed")) {
        return result;
    }
    if (int result = expect(topHit.objectIndex == 1, "topmost search order failed")) {
        return result;
    }

    TDVecModelHitResult nodeHit = model.FindNodeAt({100.0, 10.0}, 2.0);
    if (int result = expect(nodeHit.IsNodeHit(), "node hit failed")) {
        return result;
    }
    if (int result = expect(nodeHit.objectIndex == 1, "node hit index failed")) {
        return result;
    }
    if (int result = expect(nodeHit.lineHit.kind == TDVecLineHitKind::EndNode, "node hit kind failed")) {
        return result;
    }

    TDVecModelHitResult miss = model.FindObjectAt({50.0, 30.0}, 2.0);
    if (int result = expect(!miss.IsHit(), "miss unexpectedly hit object")) {
        return result;
    }

    if (int result = expect(model.SelectOnlyObject(0), "SelectOnlyObject returned false")) {
        return result;
    }
    if (int result = expect(!model.IsChanged(), "selection dirtied model")) {
        return result;
    }
    if (int result = expect(firstLine->GetSelect(), "first line not selected")) {
        return result;
    }
    if (int result = expect(!secondLine->GetSelect(), "second line unexpectedly selected")) {
        return result;
    }
    if (int result = expect(model.GetSelectedObjectIndex() == 0, "selected object index failed")) {
        return result;
    }
    if (int result = expect(model.GetSelectedObject() == firstLine, "selected object pointer failed")) {
        return result;
    }
    if (int result = expect(model.CountSelectedObjects() == 1, "selected count failed")) {
        return result;
    }

    if (int result = expect(model.SelectObject(1), "SelectObject returned false")) {
        return result;
    }
    if (int result = expect(model.CountSelectedObjects() == 2, "multi-select count failed")) {
        return result;
    }
    if (int result = expect(model.GetLockResizeForSelectedObjects() == 0, "initial selected resize lock count failed")) {
        return result;
    }
    if (int result = expect(model.GetLockPositionForSelectedObjects() == 0, "initial selected position lock count failed")) {
        return result;
    }
    if (int result = expect(model.SetLockResizeForSelectedObjects(true), "SetLockResizeForSelectedObjects returned false")) {
        return result;
    }
    if (int result = expect(model.IsChanged(), "lock resize did not dirty model")) {
        return result;
    }
    model.SetChanged(false);
    if (int result = expect(model.SetLockPositionForSelectedObjects(true), "SetLockPositionForSelectedObjects returned false")) {
        return result;
    }
    if (int result = expect(model.IsChanged(), "lock position did not dirty model")) {
        return result;
    }
    model.SetChanged(false);
    if (int result = expect(firstLine->GetLockResize() && secondLine->GetLockResize(), "selected resize locks were not set")) {
        return result;
    }
    if (int result = expect(firstLine->GetLockPosition() && secondLine->GetLockPosition(), "selected position locks were not set")) {
        return result;
    }
    if (int result = expect(model.GetLockResizeForSelectedObjects() == 2, "selected resize lock count after set failed")) {
        return result;
    }
    if (int result = expect(model.GetLockPositionForSelectedObjects() == 2, "selected position lock count after set failed")) {
        return result;
    }
    if (int result = expect(model.SetLockResizeForSelectedObjects(false), "SetLockResizeForSelectedObjects false returned false")) {
        return result;
    }
    if (int result = expect(model.SetLockPositionForSelectedObjects(false), "SetLockPositionForSelectedObjects false returned false")) {
        return result;
    }
    if (int result = expect(!firstLine->GetLockResize() && !secondLine->GetLockResize(), "selected resize locks were not cleared")) {
        return result;
    }
    if (int result = expect(!firstLine->GetLockPosition() && !secondLine->GetLockPosition(), "selected position locks were not cleared")) {
        return result;
    }

    {
        TDVecModel alignModel;
        auto* leftLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(10.0, 10.0, 20.0, 10.0)));
        auto* rightLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(30.0, 40.0, 50.0, 40.0)));
        alignModel.SelectObject(0);
        alignModel.SelectObject(1);
        alignModel.SetChanged(false);
        if (int result = expect(alignModel.AlignSelectedObjects(TDVecAlignMode::Left) == 1, "align left moved count failed")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(leftLine).left, extentsOf(rightLine).left, "align left failed")) {
            return result;
        }
        if (int result = expect(alignModel.IsChanged(), "align left did not dirty model")) {
            return result;
        }
    }
    {
        TDVecModel alignModel;
        auto* leftLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(10.0, 10.0, 20.0, 10.0)));
        auto* rightLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(30.0, 40.0, 50.0, 40.0)));
        alignModel.SelectObject(0);
        alignModel.SelectObject(1);
        if (int result = expect(alignModel.AlignSelectedObjects(TDVecAlignMode::Right) == 1, "align right moved count failed")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(leftLine).right, extentsOf(rightLine).right, "align right failed")) {
            return result;
        }
    }
    {
        TDVecModel alignModel;
        auto* topLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(10.0, 10.0, 20.0, 10.0)));
        auto* bottomLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(30.0, 40.0, 50.0, 40.0)));
        alignModel.SelectObject(0);
        alignModel.SelectObject(1);
        if (int result = expect(alignModel.AlignSelectedObjects(TDVecAlignMode::Top) == 1, "align top moved count failed")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(topLine).top, extentsOf(bottomLine).top, "align top failed")) {
            return result;
        }
    }
    {
        TDVecModel alignModel;
        auto* topLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(10.0, 10.0, 20.0, 10.0)));
        auto* bottomLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(30.0, 40.0, 50.0, 40.0)));
        alignModel.SelectObject(0);
        alignModel.SelectObject(1);
        if (int result = expect(alignModel.AlignSelectedObjects(TDVecAlignMode::Bottom) == 1, "align bottom moved count failed")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(topLine).bottom, extentsOf(bottomLine).bottom, "align bottom failed")) {
            return result;
        }
    }
    {
        TDVecModel alignModel;
        auto* leftLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(10.0, 10.0, 20.0, 10.0)));
        auto* rightLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(30.0, 40.0, 50.0, 40.0)));
        alignModel.SelectObject(0);
        alignModel.SelectObject(1);
        if (int result = expect(alignModel.AlignSelectedObjects(TDVecAlignMode::HorizontalCenter) == 2, "align center moved count failed")) {
            return result;
        }
        if (int result = expectEqual(leftLine->GetMidpoint().x, rightLine->GetMidpoint().x, "align center failed")) {
            return result;
        }
    }
    {
        TDVecModel alignModel;
        auto* topLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(10.0, 10.0, 20.0, 10.0)));
        auto* bottomLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(30.0, 40.0, 50.0, 40.0)));
        alignModel.SelectObject(0);
        alignModel.SelectObject(1);
        if (int result = expect(alignModel.AlignSelectedObjects(TDVecAlignMode::VerticalMiddle) == 2, "align middle moved count failed")) {
            return result;
        }
        if (int result = expectEqual(topLine->GetMidpoint().y, bottomLine->GetMidpoint().y, "align middle failed")) {
            return result;
        }
    }
    {
        TDVecModel alignModel;
        auto* movableLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(10.0, 10.0, 20.0, 10.0)));
        auto* lockedLine = dynamic_cast<TDVecLine*>(alignModel.AppendObject(new TDVecLine(30.0, 40.0, 50.0, 40.0)));
        lockedLine->SetLockPosition(true);
        alignModel.SelectObject(0);
        alignModel.SelectObject(1);
        if (int result = expect(alignModel.AlignSelectedObjects(TDVecAlignMode::Right) == 1, "align did not respect lock position moved count")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(movableLine).right, 50.0, "align right with locked anchor failed")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(lockedLine).right, 50.0, "locked object moved during align")) {
            return result;
        }
    }

    model.DeselectAll();
    if (int result = expect(!model.SetLockResizeForSelectedObjects(true), "SetLockResizeForSelectedObjects accepted empty selection")) {
        return result;
    }
    if (int result = expect(!model.SetLockPositionForSelectedObjects(true), "SetLockPositionForSelectedObjects accepted empty selection")) {
        return result;
    }
    if (int result = expect(model.SelectObjectsInArea({-1.0, -1.0}, {101.0, 1.0}, false) == 1, "SelectObjectsInArea count failed")) {
        return result;
    }
    if (int result = expect(firstLine->GetSelect() && !secondLine->GetSelect(), "SelectObjectsInArea selection failed")) {
        return result;
    }
    if (int result = expect(model.SelectObjectsInArea({-1.0, 9.0}, {101.0, 11.0}, true) == 1, "SelectObjectsInArea add count failed")) {
        return result;
    }
    if (int result = expect(firstLine->GetSelect() && secondLine->GetSelect(), "SelectObjectsInArea add selection failed")) {
        return result;
    }

    if (int result = expect(model.MakeGroup(), "MakeGroup returned false for two selected objects")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 1, "object count after MakeGroup failed")) {
        return result;
    }
    auto* group = dynamic_cast<TDVecGroup*>(model.GetObject(0));
    if (int result = expect(group != nullptr, "MakeGroup did not create TDVecGroup")) {
        return result;
    }
    if (int result = expect(group->GetSelect(), "new group was not selected")) {
        return result;
    }
    if (int result = expect(group->CountObjectsInGroup() == 2, "group object count failed")) {
        return result;
    }
    if (int result = expect(model.ResolveAllSelectedGroups(), "ResolveAllSelectedGroups returned false")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 2, "object count after ResolveAllSelectedGroups failed")) {
        return result;
    }
    if (int result = expect(dynamic_cast<TDVecLine*>(model.GetObject(0)) != nullptr && dynamic_cast<TDVecLine*>(model.GetObject(1)) != nullptr, "resolved group did not restore line objects")) {
        return result;
    }
    if (int result = expect(model.CountSelectedObjects() == 2, "resolved objects were not selected")) {
        return result;
    }

    auto* resolvedFirst = dynamic_cast<TDVecLine*>(model.GetObject(0));
    auto* resolvedSecond = dynamic_cast<TDVecLine*>(model.GetObject(1));
    if (int result = expect(resolvedFirst != nullptr && resolvedSecond != nullptr, "resolved line lookup failed")) {
        return result;
    }
    resolvedFirst->SetLockResize(true);
    resolvedFirst->SetLockPosition(true);
    if (int result = expect(!model.DeleteObject(0), "DeleteObject accepted locked object")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 2, "locked DeleteObject changed object count")) {
        return result;
    }
    if (int result = expect(model.DeleteSelectedObjects() == 1, "DeleteSelectedObjects did not skip locked selected object")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 1, "object count after locked DeleteSelectedObjects failed")) {
        return result;
    }
    if (int result = expect(model.GetObject(0) == resolvedFirst, "locked object was removed by DeleteSelectedObjects")) {
        return result;
    }
    resolvedFirst->SetLockResize(false);
    resolvedFirst->SetLockPosition(false);

    if (int result = expect(model.DeleteSelectedObjects() == 1, "DeleteSelectedObjects count failed")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 0, "object count after DeleteSelectedObjects failed")) {
        return result;
    }

    model.AppendObject(new TDVecLine(0.0, 0.0, 1.0, 1.0));
    model.AppendObject(new TDVecLine(2.0, 2.0, 3.0, 3.0));
    if (int result = expect(model.DeleteObject(0), "DeleteObject returned false")) {
        return result;
    }
    if (int result = expect(model.CountObjects() == 1, "object count after DeleteObject failed")) {
        return result;
    }
    if (int result = expect(!model.DeleteObject(9), "DeleteObject accepted invalid index")) {
        return result;
    }

    {
        TDVecModel historyModel;
        TDVecEditCad historyEditor;
        TDViewOperationManager manager(&historyModel, &historyEditor);
        auto* first = dynamic_cast<TDVecLine*>(historyModel.AppendObject(new TDVecLine(10.0, 0.0, 20.0, 0.0)));
        auto* second = dynamic_cast<TDVecLine*>(historyModel.AppendObject(new TDVecLine(30.0, 10.0, 50.0, 10.0)));
        historyModel.SelectObject(0);
        historyModel.SelectObject(1);
        historyModel.SetChanged(false);
        if (int result = expect(!manager.CanUndo() && !manager.CanRedo(), "new manager history was not empty")) {
            return result;
        }
        if (int result = expect(manager.AlignSelectedObjects(TDVecAlignMode::Right), "history align command failed")) {
            return result;
        }
        if (int result = expect(manager.CanUndo() && !manager.CanRedo(), "align did not create undo history")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(first).right, extentsOf(second).right, "history align did not apply")) {
            return result;
        }
        if (int result = expect(manager.Undo(), "undo failed")) {
            return result;
        }
        first = dynamic_cast<TDVecLine*>(historyModel.GetObject(0));
        second = dynamic_cast<TDVecLine*>(historyModel.GetObject(1));
        if (int result = expect(first != nullptr && second != nullptr, "undo restored invalid objects")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(first).right, 20.0, "undo did not restore first line")) {
            return result;
        }
        if (int result = expect(manager.CanRedo(), "undo did not create redo history")) {
            return result;
        }
        if (int result = expect(manager.Redo(), "redo failed")) {
            return result;
        }
        first = dynamic_cast<TDVecLine*>(historyModel.GetObject(0));
        second = dynamic_cast<TDVecLine*>(historyModel.GetObject(1));
        if (int result = expect(first != nullptr && second != nullptr, "redo restored invalid objects")) {
            return result;
        }
        if (int result = expectEqual(extentsOf(first).right, extentsOf(second).right, "redo did not restore aligned state")) {
            return result;
        }
    }

    return 0;
}
