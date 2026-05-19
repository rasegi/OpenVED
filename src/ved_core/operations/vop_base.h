//---------------------------------------------------------------------------
// HEADER:  View-Operation
//
// DESCR:   definiert die bases klasse für Kapselung die View-Operationen
//          diese Klasse ist abstract.!
//          die alle andere View-Operationen werden von diese Klasse abgeleitet
//
//  ...
//
// HISTORY:
// Version   Datum       Autor       Wo/Was/Warum
// V1.0      05.05.99    tehrani     Ersterstellung
// V1.1      27.09.99    tehrani     TDVOPExternalVariables
//---------------------------------------------------------------------------

#ifndef __VOP_BASE_H
#define __VOP_BASE_H

#include "vec_maindef.h"
#include "vec_object.h"

#ifndef __fastcall
#define __fastcall
#endif

//---------------------------------------------------------------------------
class TDVecEditCad ;
class TDVecModel;
class TDViewOperation;
class TDViewOperationManager;
//---------------------------------------------------------------------------
class TDVOPExternalVariables
{
public:
    TDVOPExternalVariables(TDViewOperation* pParentOperation);              // ! Constructor with Parameter
    TDVOPExternalVariables(const TDVOPExternalVariables&);                  // ! Copy Contructor
    TDVOPExternalVariables& operator = (const TDVOPExternalVariables&);     // ! assignment operator
    virtual ~TDVOPExternalVariables() = 0;                                  // ! virtuale Destruktor
    virtual TDVOPExternalVariables* Clone(void) const = 0 ;                 // ! Clone-MEthode

    const   TDViewOperation* GetParentOperation(void);

    void    SendUpdateToOPManager(void);
    void    SendUpdateToParrentOP(void);

protected:
    void SetParentOperation(TDViewOperation* pParentOperation);
    TDViewOperation*    mpParentOperation;
private:
    TDVOPExternalVariables();                                               // ! Default Constructor
};
//---------------------------------------------------------------------------
// abstrakte basses Klasse für alle View-Operationen
//---------------------------------------------------------------------------
class   TDViewOperation
{
public:
    // ! Constructor with Parameter
    TDViewOperation(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager);
    TDViewOperation(const TDViewOperation&);                                // ! Copy Contructor
    TDViewOperation& operator = (const TDViewOperation&);                   // ! assignment operator
    virtual ~TDViewOperation() = 0;                                         // ! virtuale Destruktor
    virtual TDViewOperation* Clone(void) const = 0 ;                        // ! Clone-MEthode
    //-*-*-*-*-*-*-*
    virtual void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPMouseUp  (TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey) = 0;
    virtual void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey) = 0;
    //-*-*-*-*-*-*-*
    virtual void __fastcall IniInteractivePoints(void) = 0;
    TDViewOperationState    GetState();

    bool    SetVecModel(TDVecModel* pVecModel);
    bool    SetVecEditCad(TDVecEditCad* pVecEditCad);
    bool    SetOperationManager(TDViewOperationManager* pParentOperationManager);

    bool                    SetVOPExternalVariables(TDVOPExternalVariables* pVariables);
    TDVOPExternalVariables* GetVOPExternalVariables(void);

    virtual void    Initialize(void){};
    virtual void    ExtVarIsChanged(void){};
    void            ApplyExternalVariablesChanged(void);
    void            SendUpdateExtVarToManager(void);
    void            Send_OpenDialogMessage_ToManager(void);
protected:
    TDVecModel*                 mpVecModel;
    TDVecEditCad*              mpVecEditCad;
    TDViewOperationManager*     mpParentOperationManager;
    TDVOPExternalVariables*     mpVOPExternalVariables;

    bool    ISActiveChangeToDefOPAfterFinish(void);
    void    SetState(TDViewOperationState eState, bool* pbDestroyed = NULL);
private:
    TDViewOperation();                                                      // ! default Contructor
    TDViewOperationState    meOperationState;
};
//---------------------------------------------------------------------------
// abstrakte basses Klasse für alle View-Operationen-Create
//---------------------------------------------------------------------------
class   TDVOCreate : public TDViewOperation
{
public:
     // ! Constructor with Parameter
    TDVOCreate(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager);
    TDVOCreate(const TDVOCreate&);                                      // ! Copy Contructor
    TDVOCreate& operator = (const TDVOCreate&);                         // ! assignment operator
    virtual ~TDVOCreate() = 0;                                          // ! virtuale Destruktor
    //-*-*-*-*-*-*-*
    virtual void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPMouseUp  (TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey) = 0;
    virtual void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey) = 0;
    //-*-*-*-*-*-*-*
    void __fastcall IniInteractivePoints(void);
protected:
    TDVOCreate();                                                       // ! default Contructor
    // Dtaen für temporäre und interaktive Objektzeichnung
    TDMatPoint          mPtBeg;
    TDMatPoint          mPtEnd;
    TDMatPoint          mPtPoint;
    int                 mClick;         // Anzahl von Mouse-Click
};
//---------------------------------------------------------------------------
// DESC. :  abstrakte basses Klasse für alle View-Operationen-Manipulate
//---------------------------------------------------------------------------
class   TDVOModify : public TDViewOperation
{
public:
    // ! Constructor with Parameter
    TDVOModify(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager);
    TDVOModify(const TDVOModify&);                                      // ! Copy Contructor
    TDVOModify& operator = (const TDVOModify&);                         // ! assignment operator
    virtual ~TDVOModify() = 0;                                          // ! virtuale Destruktor
    //-*-*-*-*-*-*-*
    virtual void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPMouseUp  (TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y) = 0;
    virtual void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey) = 0;
    virtual void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey) = 0;
    //-*-*-*-*-*-*-*
    void __fastcall IniInteractivePoints(void);
protected:
    TDVOModify();                                                       // ! default Contructor
    // Dtaen für temporäre und interaktive Objekt-Manipulationen
    double              XMouseStart;    // Anfang Koordinaten für Drag & Drop von Mouse
    double              YMouseStart;
    double              XMouseJet;      // Vorherige Koordinaten für Drag & Drop von Mouse
    double              YMouseJet;
};
//---------------------------------------------------------------------------
#endif
