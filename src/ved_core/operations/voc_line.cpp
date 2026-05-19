//---------------------------------------------------------------------------
// MODULE: View-Operation-Create-Line
//
// DESCR:   implementiert die klasse für Kapselung die View-Operationen-Create-Line
//          diese Klasse ist von TDVOCreate abgeleitet
//
//  ...
//
// HISTORY:
// Version   Datum       Autor       Wo/Was/Warum
// V1.0.00    06.05.99    rasegi      Ersterstellung
// V1.1.00    27.09.99    rasegi      Communication-Interface To Parrent-GUI
//---------------------------------------------------------------------------
#define __VOC_LINE_CPP

#include "voc_line.h"

#include "vec_line.h"

//===========================================================================
//  Kontruktur und Destruktur
//===========================================================================
TDVOCLine::TDVOCLine(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager)
: TDVOCreate( pVecModel ,  pVecEditCad , pParentOperationManager)
{
    mbLineOK = false;
    mpObjLine = new TDVecLine();
    IniInteractivePoints();
    SetState(OSTATE_STARTED);
    mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_1);

    mpVOPExternalVariables = new TDVocLineExtVar(this);
    mpVocLineExtVar = dynamic_cast<TDVocLineExtVar*>(mpVOPExternalVariables);
}

TDVOCLine::~TDVOCLine()
{
    delete mpObjLine;
    delete mpVOPExternalVariables;
}
//---------------------------------------------------------------------------
// ! Copy Constructor
//---------------------------------------------------------------------------
TDVOCLine::TDVOCLine(const TDVOCLine& OldOperation)
: TDVOCreate(OldOperation)
{
    mpObjLine = OldOperation.mpObjLine;
    mMatLine = OldOperation.mMatLine;
    mbLineOK = OldOperation.mbLineOK;
}
//---------------------------------------------------------------------------
// ! assignment operator
//---------------------------------------------------------------------------
TDVOCLine& TDVOCLine::operator = (const TDVOCLine& OldOperation)
{
    if(this == &OldOperation) return *this;
    TDVOCreate::operator=(OldOperation);
    mpObjLine = OldOperation.mpObjLine;
    mMatLine = OldOperation.mMatLine;
    mbLineOK = OldOperation.mbLineOK;
    return *this;
}
//---------------------------------------------------------------------------
// ! Clone Methode
//---------------------------------------------------------------------------
TDVOCLine* TDVOCLine::Clone(void) const
{
    return(new TDVOCLine(*this));
}
//---------------------------------------------------------------------------
//  DESC. :
//---------------------------------------------------------------------------
void __fastcall TDVOCLine::OPMouseDown(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y)
{
    switch(GetState())
    {
        case OSTATE_UNKNOWN :
            break;
        case OSTATE_STARTED :
                mbLineOK = false;
                SetState(OSTATE_RUNNING);
                IniInteractivePoints();
                if(mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_LINE_1)
                    mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_1);
            break;
        case OSTATE_RUNNING :
            break;
        case OSTATE_FINISHED:
                mbLineOK = false;
                SetState(OSTATE_RUNNING);
                IniInteractivePoints();
                if(mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_LINE_1)
                    mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_1);
            break;
        case OSTATE_ABORTED :
            break;
    }

    if(Button == VIRTMOUSEBTM_RIGHT)
    {
        mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_1);
        mpVecEditCad->TmpClear();
        mpVecEditCad->ViewsRefresh();
        IniInteractivePoints();
        SetState(OSTATE_STARTED);
    }

    if(Button == VIRTMOUSEBTM_LEFT)
    {
        if(mClick > 2)
            mClick = 0;
        mClick+=1;
        switch(mClick)
        {
            case 1:
                    mPtBeg.x = X ;
                    mPtBeg.y = Y ;
                    mPtEnd.x = X ;
                    mPtEnd.y = Y ;
                break;
            case 2:
                    if(mbLineOK)
                    {
                        mPtEnd.x = X ;
                        mPtEnd.y = Y ;
                    }
                    else
                    {
                        mClick = 1;
                        mpVecEditCad->Beep();
                    }
                break;
        }
    }
}
//---------------------------------------------------------------------------
//  DESC. :
//---------------------------------------------------------------------------
void __fastcall TDVOCLine::OPMouseUp(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y)
{
    switch(mClick)
    {
        case 1:
            break;
        case 2:
        {
            if(mbLineOK)
            {
                TDVecLine*  pObjLine;
                pObjLine = new TDVecLine();
                pObjLine->SetLine(&mMatLine);
                pObjLine->SetSelect(false);
                pObjLine->SetColor(mpVecModel->GetDefaultColor());
                pObjLine->Initialize();
                mpVecModel->AppendObject(pObjLine);
                IniInteractivePoints();
                mpVecEditCad->TmpClear();
                mpVecEditCad->ViewsRefresh();
                mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_1);

                bool    bDestroyed;
                SetState(OSTATE_FINISHED , &bDestroyed);
                if(bDestroyed) return;
            }
            else
            {
                mClick = 1;
                mpVecEditCad->Beep();
            }
        }
        break;
    }//switch()
}
//---------------------------------------------------------------------------
//  DESC. :
//---------------------------------------------------------------------------
void __fastcall TDVOCLine::OPMouseMove(TDOPVirtMouseButton Button, TDOPVirtKeyState Shift, double X, double Y)
{
     if(mClick == 1 && Button==VIRTMOUSEBTM_LEFT)
         mClick = 2 ;

     if(mClick >= 1)
     {
         mPtEnd.x = X ;
         mPtEnd.y = Y ;
         mMatLine.Create(mPtBeg , mPtEnd);
         double nToleranz = mpVecEditCad->GetActiveGraphicEngine()->GetMinimumDistance();
         if(mMatLine.Length() > nToleranz)
         {
             mbLineOK = true;
             if(mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_LINE_2)
                mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_2);
         }
         else
         {
             mbLineOK = false;
             if(mpVecEditCad->GetUsedCursor() != VECVIEW_CREATE_LINE_1)
                mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_1);
         }

         mpVocLineExtVar->SetMatLine(mMatLine);
         mpVocLineExtVar->SetMatPoint(mPtEnd);
         mpVocLineExtVar->SendUpdateToOPManager();
         mpObjLine->SetLine(&mMatLine);
         mpObjLine->SetSelect(false) ;

         mpVecEditCad->TmpAppend(mpObjLine , true);
         SetState(OSTATE_RUNNING);
     }
     else
         mpVecEditCad->UseCursor(VECVIEW_CREATE_LINE_1);

     if(mClick == 1 && Button==VIRTMOUSEBTM_LEFT)
         mClick = 2 ;

}
//---------------------------------------------------------------------------
//  DESC. :
//---------------------------------------------------------------------------
void __fastcall TDVOCLine::OPKeyDown(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey)
{
}
//---------------------------------------------------------------------------
//  DESC. :
//---------------------------------------------------------------------------
void __fastcall TDVOCLine::OPKeyUp(TDOPVirtKey eVirtualKey , TDOPVirtKeyState StateKey)
{
}
//===========================================================================
//
//===========================================================================
//---------------------------------------------------------------------------
//  DESC. : Constructor with Parameter
//---------------------------------------------------------------------------
TDVocLineExtVar::TDVocLineExtVar(TDViewOperation* pParentOperation)
: TDVOPExternalVariables(pParentOperation)
{
    mMatLine.InitialWithNULL();
    mMatPoint.Create(0.0 , 0.0);
}
//---------------------------------------------------------------------------
//  DESC. : Copy Contructor
//---------------------------------------------------------------------------
TDVocLineExtVar::TDVocLineExtVar(const TDVocLineExtVar& OldExtVar)
: TDVOPExternalVariables(OldExtVar)
{
    mMatLine = OldExtVar.mMatLine;
    mMatPoint = OldExtVar.mMatPoint;
}
//---------------------------------------------------------------------------
//  DESC. : default Destructor
//---------------------------------------------------------------------------
TDVocLineExtVar::~TDVocLineExtVar()
{
}
//---------------------------------------------------------------------------
//  DESC. : assignment operator
//---------------------------------------------------------------------------
TDVocLineExtVar& TDVocLineExtVar::operator = (const TDVocLineExtVar& OldExtVar)
{
    if(this == &OldExtVar) return *this;
    TDVOPExternalVariables::operator=(OldExtVar);
    mMatLine = OldExtVar.mMatLine;
    mMatPoint = OldExtVar.mMatPoint;
    return *this;
}
//---------------------------------------------------------------------------
//  DESC. : Clone Methode
//---------------------------------------------------------------------------
TDVocLineExtVar* TDVocLineExtVar::Clone(void) const
{
    return(new TDVocLineExtVar(*this));
}
//---------------------------------------------------------------------------
//  DESC. : Set- und Get Methode for Interne-Variables
//---------------------------------------------------------------------------
void TDVocLineExtVar::SetMatLine(TDMatLine MatLine)
{
    mMatLine.Create(&MatLine);
}

TDMatLine TDVocLineExtVar::GetMatLine(void)
{
    return(mMatLine);
}

void TDVocLineExtVar::SetMatPoint(TDMatPoint MatPoint)
{
    mMatPoint.Create(&MatPoint);
}
TDMatPoint TDVocLineExtVar::GetMatPoint(void)
{
    return(mMatPoint);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//EOF
