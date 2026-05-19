//---------------------------------------------------------------------------
// MODULE:  View-Operation
//
// DESCR:   implementiert die bases klasse für Kapselung die View-Operationen
//          als abstracte bases  Klasse.
//
//  ...
//
// HISTORY:
// Version   Datum       Autor       Wo/Was/Warum
// V1.0      05.05.99    tehrani     Ersterstellung
// V1.1      27.09.99    tehrani     TDVOPExternalVariables
//---------------------------------------------------------------------------

#define __VOP_BASE_CPP

#include "vop_base.h"
#include "vec_edit_cad.h"
#include "vop_manager.h"
#include "vec_model.h"

//==============================================================================
// Implementierung von TDVOPExternalVariables
//==============================================================================
//------------------------------------------------------------------------------
// ! Contructor with Parameter
//------------------------------------------------------------------------------
TDVOPExternalVariables::TDVOPExternalVariables(TDViewOperation* pParentOperation)
{
    SetParentOperation(pParentOperation);
}
//------------------------------------------------------------------------------
// ! Copy Contructor
//------------------------------------------------------------------------------
TDVOPExternalVariables::TDVOPExternalVariables(const TDVOPExternalVariables& VOPExtVar)
{
    mpParentOperation = VOPExtVar.mpParentOperation;
}
//---------------------------------------------------------------------------
// ! assignment operator
//---------------------------------------------------------------------------
TDVOPExternalVariables& TDVOPExternalVariables::operator = (const TDVOPExternalVariables& VOPExtVar)
{
    if(this == &VOPExtVar) return *this;
    mpParentOperation = VOPExtVar.mpParentOperation;
    return *this;
}
//------------------------------------------------------------------------------
// ! virtualle Destruktor
//------------------------------------------------------------------------------
TDVOPExternalVariables::~TDVOPExternalVariables()
{
}
//------------------------------------------------------------------------------
// DESCR:
//------------------------------------------------------------------------------
const TDViewOperation* TDVOPExternalVariables::GetParentOperation(void)
{
    if(!this) return(NULL);
    return(mpParentOperation);
}
void TDVOPExternalVariables::SetParentOperation(TDViewOperation*pParentOperation)
{
    if(!this) return;
    mpParentOperation = pParentOperation;
}
//------------------------------------------------------------------------------
// DESCR:
//------------------------------------------------------------------------------
void TDVOPExternalVariables::SendUpdateToOPManager(void)
{
    if(!this) return;
    mpParentOperation->SendUpdateExtVarToManager();
}
void TDVOPExternalVariables::SendUpdateToParrentOP(void)
{
    if(!this) return;
    mpParentOperation->ApplyExternalVariablesChanged();
}

//==============================================================================
// Implementierung von TDViewOperation
//==============================================================================
//------------------------------------------------------------------------------
// DESCR:
//   Contructor with Parameter
//------------------------------------------------------------------------------
TDViewOperation::TDViewOperation(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager)
{
    SetVecModel(pVecModel);
    SetVecEditCad(pVecEditCad);
    SetOperationManager(pParentOperationManager);
    mpVOPExternalVariables = NULL;
    meOperationState = OSTATE_UNKNOWN;
    pVecEditCad->SetVisualNodes(true);
    pVecEditCad->SetVisualFrame(false);
}
//------------------------------------------------------------------------------
// DESCR:
//  Copy Contructor
//------------------------------------------------------------------------------
TDViewOperation::TDViewOperation(const TDViewOperation& ViewOperation)
{
    mpVecModel = ViewOperation.mpVecModel;
    mpVecEditCad = ViewOperation.mpVecEditCad;
    meOperationState = ViewOperation.meOperationState;
    mpVOPExternalVariables = ViewOperation.mpVOPExternalVariables->Clone();
    mpParentOperationManager = ViewOperation.mpParentOperationManager;
}
//---------------------------------------------------------------------------
// ! assignment operator
//---------------------------------------------------------------------------
TDViewOperation& TDViewOperation::operator = (const TDViewOperation& ViewOperation)
{
    if(this == &ViewOperation) return *this;
    mpVecModel = ViewOperation.mpVecModel;
    mpVecEditCad = ViewOperation.mpVecEditCad;
    meOperationState = ViewOperation.meOperationState;
    mpVOPExternalVariables = ViewOperation.mpVOPExternalVariables;
    mpParentOperationManager = ViewOperation.mpParentOperationManager;
    return *this;
}
//------------------------------------------------------------------------------
// DESCR:
//  virtualle Destruktor
//------------------------------------------------------------------------------
TDViewOperation::~TDViewOperation()
{
}
//------------------------------------------------------------------------------
// DESCR:
//  registriert die VecModel bei die Operation
//------------------------------------------------------------------------------
bool TDViewOperation::SetVecModel(TDVecModel* pVecModel)
{
    if(!this) return(false);
bool    result;
    if(pVecModel)
    {
        mpVecModel = pVecModel;
        result = true;
    }
    else
    {
        result = false;
    }

return(result);
}
//------------------------------------------------------------------------------
// DESCR:
//  registriert die VecEditCad bei die Operation
//------------------------------------------------------------------------------
bool TDViewOperation::SetVecEditCad(TDVecEditCad* pVecEditCad)
{
    if(!this) return(false);
bool    result;
    if(pVecEditCad)
    {
        mpVecEditCad = pVecEditCad;
        result = true;
    }
    else
    {
        result = false;
    }

return(result);
}
//------------------------------------------------------------------------------
// DESCR:
//  registriert die TDViewOperationManager bei die Operation
//------------------------------------------------------------------------------
bool TDViewOperation::SetOperationManager(TDViewOperationManager* pParentOperationManager)
{
    if(!this) return(false);
bool    result;
    if(pParentOperationManager)
    {
        mpParentOperationManager = pParentOperationManager;
        result = true;
    }
    else
    {
        result = false;
    }

return(result);
}
//------------------------------------------------------------------------------
// DESCR:
//  Set- und Get-Methode für Status von Operation
//------------------------------------------------------------------------------
void TDViewOperation::SetState(TDViewOperationState eState, bool* pbDestroyed)
{
    if(!this) return;
    meOperationState = eState;
    if(mpParentOperationManager->GetActiveChangeToDefOPAfterFinish() && eState == OSTATE_FINISHED)
    {
        if(mpParentOperationManager->GetAktiveOperationType() != mpParentOperationManager->GetDefaultOperation())
        {
            mpParentOperationManager->AktivateDefaultOperation();
            if(pbDestroyed) *pbDestroyed = true;
        }
        return;
    }
    else if(mpParentOperationManager->GetActiveChangeToDefOPAfterAbort()  && eState == OSTATE_ABORTED)
    {
        mpParentOperationManager->AktivateDefaultOperation();
        if(pbDestroyed) *pbDestroyed = true;
        return;
    }
    else
    {
        if(pbDestroyed) *pbDestroyed = false;
        return;
    }
}

bool TDViewOperation::ISActiveChangeToDefOPAfterFinish(void)
{
    if(!this) return(false);
    return(mpParentOperationManager->GetActiveChangeToDefOPAfterFinish());
}


TDViewOperationState TDViewOperation::GetState()
{
    if(!this || !meOperationState) return(OSTATE_UNKNOWN);
    return(meOperationState);
}


//------------------------------------------------------------------------------
bool TDViewOperation::SetVOPExternalVariables(TDVOPExternalVariables* pVariables)
{
    if(!this) return(false);
bool    bResult;
    if(pVariables)
    {
        mpVOPExternalVariables = pVariables;
        bResult = true;
    }
    else
        {bResult = false;}

    return(bResult);
}

TDVOPExternalVariables* TDViewOperation::GetVOPExternalVariables(void)
{
    if(!this) return(NULL);
    return(mpVOPExternalVariables);
}

void TDViewOperation::ApplyExternalVariablesChanged(void)
{
    if(!this) return;
    if(mpParentOperationManager)
        {mpParentOperationManager->ApplyExternalVariablesChanged(this);}
    else
        {ExtVarIsChanged();}
}

//------------------------------------------------------------------------------
void TDViewOperation::SendUpdateExtVarToManager(void)
{
    if(!this) return;
    mpParentOperationManager->SendUpdateExtVarToParent();
}

//------------------------------------------------------------------------------
void TDViewOperation::Send_OpenDialogMessage_ToManager(void)
{
    if(!this) return;
    mpParentOperationManager->Send_OpenDialogMessage_ToParent();
}
//==============================================================================
//  Implementierung von  TDVOCreate
//==============================================================================
//------------------------------------------------------------------------------
// DESCR:
//  Contructor with Parameter
//------------------------------------------------------------------------------
TDVOCreate::TDVOCreate(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager)
: TDViewOperation( pVecModel ,  pVecEditCad , pParentOperationManager)
{
}
//------------------------------------------------------------------------------
// ! Copy Contructor
//------------------------------------------------------------------------------
TDVOCreate::TDVOCreate(const TDVOCreate& VOCreate)
: TDViewOperation(VOCreate)
{
    mPtBeg = VOCreate.mPtBeg;
    mPtEnd = VOCreate.mPtEnd;
    mPtPoint = VOCreate.mPtPoint;
    mClick = VOCreate.mClick;
}
//---------------------------------------------------------------------------
// ! assignment operator
//---------------------------------------------------------------------------

TDVOCreate& TDVOCreate::operator = (const TDVOCreate& VOCreate)
{
    if(this == &VOCreate) return *this;
    TDViewOperation::operator=(VOCreate);
    mPtBeg = VOCreate.mPtBeg;
    mPtEnd = VOCreate.mPtEnd;
    mPtPoint = VOCreate.mPtPoint;
    mClick = VOCreate.mClick;
    return *this;
}
//------------------------------------------------------------------------------
// ! default Destruktor
//------------------------------------------------------------------------------
TDVOCreate::~TDVOCreate()
{
}
//------------------------------------------------------------------------------
// DESCR:
//  Initializiert die Temporär-Interaktive Variable für Create Operationen
//------------------------------------------------------------------------------
void __fastcall TDVOCreate::IniInteractivePoints(void)
{
    mPtBeg.x = 0;
    mPtBeg.y = 0;
    mPtEnd.x = 0;
    mPtEnd.y = 0;
    mPtPoint.x = 0;
    mPtPoint.y = 0;
    mClick = 0;
    if (mpVecEditCad) {
        mpVecEditCad->ClearInteractiveNodes();
    }
}
//==============================================================================
//  Implementierung von  TDVOModify
//==============================================================================
//------------------------------------------------------------------------------
// DESCR:
//  Contructor with Parameter
//------------------------------------------------------------------------------
TDVOModify::TDVOModify(TDVecModel* pVecModel , TDVecEditCad* pVecEditCad , TDViewOperationManager* pParentOperationManager)
: TDViewOperation( pVecModel ,  pVecEditCad , pParentOperationManager)
{
}
//------------------------------------------------------------------------------
// ! Copy Contructor
//------------------------------------------------------------------------------
TDVOModify::TDVOModify(const TDVOModify& VOModify)
: TDViewOperation(VOModify)
{
    XMouseStart = VOModify.XMouseStart;
    YMouseStart = VOModify.YMouseStart;
    XMouseJet = VOModify.XMouseJet;
    YMouseJet = VOModify.YMouseJet;
}
//---------------------------------------------------------------------------
// ! assignment operator
//---------------------------------------------------------------------------

TDVOModify& TDVOModify::operator = (const TDVOModify& VOModify)
{
    if(this == &VOModify) return *this;
    TDViewOperation::operator=(VOModify);
    XMouseStart = VOModify.XMouseStart;
    YMouseStart = VOModify.YMouseStart;
    XMouseJet = VOModify.XMouseJet;
    YMouseJet = VOModify.YMouseJet;
    return *this;
}
//------------------------------------------------------------------------------
// ! default Destruktor
//------------------------------------------------------------------------------
TDVOModify::~TDVOModify()
{
}
//------------------------------------------------------------------------------
// DESCR:
//  Initializiert die Temporär-Interaktive Variable für Modify Operationen
//------------------------------------------------------------------------------
void __fastcall TDVOModify::IniInteractivePoints(void)
{
    XMouseStart = 0;
    YMouseStart = 0;
    XMouseJet = 0;
    YMouseJet = 0;
    if (mpVecEditCad) {
        mpVecEditCad->ClearInteractiveNodes();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//EOF
