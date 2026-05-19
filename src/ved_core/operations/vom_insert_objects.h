//---------------------------------------------------------------------------
// HEADER: View-Operation-Modify-Insert-Objects
//---------------------------------------------------------------------------
#ifndef __VOM_INSERT_OBJECTS_H
#define __VOM_INSERT_OBJECTS_H

#include "vop_base.h"

#include <memory>
#include <vector>

class TDVecGroup;

class TDVOMInsertObjectsExtVar : public TDVOPExternalVariables {
public:
    TDVOMInsertObjectsExtVar(TDViewOperation* pParentOperation);
    TDVOMInsertObjectsExtVar(const TDVOMInsertObjectsExtVar&);
    ~TDVOMInsertObjectsExtVar() override = default;
    TDVOMInsertObjectsExtVar& operator=(const TDVOMInsertObjectsExtVar&);
    TDVOMInsertObjectsExtVar* Clone() const override;

    bool AppendObject(TDVecObject* pObject);
    std::unique_ptr<TDVecGroup> MakeTempGroupFromObjects() const;

protected:
    friend class TDVOMInsertObjects;
    std::vector<std::unique_ptr<TDVecObject>> objectsForInserting_;
    void MoveObjects(double X, double Y);

private:
    TDVOMInsertObjectsExtVar();
};

class TDVOMInsertObjects : public TDVOModify {
public:
    TDVOMInsertObjects(TDVecModel* pVecModel, TDVecEditCad* pVecEditCad, TDViewOperationManager* pParentOperationManager);
    TDVOMInsertObjects(const TDVOMInsertObjects&);
    ~TDVOMInsertObjects() override;
    TDVOMInsertObjects& operator=(const TDVOMInsertObjects&);
    TDVOMInsertObjects* Clone() const override;

    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) override;
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey, TDOPVirtKeyState StateKey) override;

    void Initialize() override;
    void ExtVarIsChanged() override;

private:
    TDVOMInsertObjects();

    TDVOMInsertObjectsExtVar* mpVOMInsertObjectsExtVar;
    std::unique_ptr<TDVecGroup> mpTmpVecGroup;
    std::unique_ptr<TDVecObject> mpTmpObject;
    bool mbGroupMove;
    bool mbDragOK;
};

#endif
