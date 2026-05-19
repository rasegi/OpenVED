//---------------------------------------------------------------------------
// HEADER: View-Operation-Create-Line             
//
// DESCR:   definiert die klasse für Kapselung die View-Operationen-Create-Line
//          diese Klasse ist von TDVOCreate abgeleitet
//
//  ...
//
// HISTORY:
// Version   Datum       Autor       Wo/Was/Warum
// V1.0.00    06.05.99    rasegi      Ersterstellung
// V1.1.00    28.09.99    rasegi      Communication-Interface To Parrent-GUI
//---------------------------------------------------------------------------
#ifndef __VOC_LINE_H
#define __VOC_LINE_H
//---------------------------------------------------------------------------
#include "vop_base.h"
#include "vec_edit_cad.h"
//---------------------------------------------------------------------------
class TDVecLine;

class TDVocLineExtVar : public TDVOPExternalVariables
{
public:
    TDVocLineExtVar(TDViewOperation* pParentOperation);     // ! Constructor with Parameter
    TDVocLineExtVar(const TDVocLineExtVar&);                // ! Copy Contructor
    ~TDVocLineExtVar();                                     // ! default Destructor
    TDVocLineExtVar& operator = (const TDVocLineExtVar&);   // ! assignment operator
    TDVocLineExtVar* Clone(void) const;                     // ! Clone Methode

    void    SetMatLine(TDMatLine MatLine);
    TDMatLine   GetMatLine(void);
    void    SetMatPoint(TDMatPoint MatPoint);
    TDMatPoint  GetMatPoint(void);
private:
    TDVocLineExtVar();                                      // ! default Constructor

    TDMatLine   mMatLine;
    TDMatPoint  mMatPoint;
};

//---------------------------------------------------------------------------
class   TDVOCLine : public TDVOCreate
{
public:
    TDVOCLine(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager);
    TDVOCLine(const TDVOCLine&);                                    // ! Copy Contructor
    ~TDVOCLine();                                                   // ! default Destructor
    TDVOCLine& operator = (const TDVOCLine&);                       // ! assignment operator
    TDVOCLine* Clone(void) const;                                   // ! Clone Methode

    //-*-*-*-*-*-*-*
    void __fastcall OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);
    void __fastcall OPMouseUp  (TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);
    void __fastcall OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y);
    void __fastcall OPKeyDown(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey);
    void __fastcall OPKeyUp(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey);
    //-*-*-*-*-*-*-*

private:
    TDVOCLine();                                                    // ! default Constructor

    TDVocLineExtVar*    mpVocLineExtVar;

    TDVecLine*          mpObjLine;
    TDMatLine           mMatLine;
    bool                mbLineOK;
};
//---------------------------------------------------------------------------
#endif
