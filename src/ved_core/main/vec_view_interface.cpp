//---------------------------------------------------------------------------
// MODULE:  Interface für dir View GUI-Komponente
//
// DESCR:
//
//
// HISTORY:
// Version   Datum       Autor       Wo/Was/Warum
//   1.0.0     28.08.00    tehrani     Erstellung
//---------------------------------------------------------------------------

#define __VEC_VIEW_INTERFACE_CPP

#include "vec_view_interface.h"

//---------------------------------------------------------------------------
TDVecViewInterfaceBase::TDVecViewInterfaceBase()
    : mpGE(nullptr) {
}

TDVecViewInterfaceBase::~TDVecViewInterfaceBase()
{}

TDGraphicEngine* TDVecViewInterfaceBase::GetGraphicEngine(void)
{
    if(!mpGE) return(NULL);
    return(mpGE);
}


void TDVecViewInterfaceBase::SetGraphicEngine(TDGraphicEngine* pGE)
{
    mpGE = pGE;
}

//---------------------------------------------------------------------------
// wegen kurzere Name und kompatibilität
//---------------------------------------------------------------------------
void TDVecViewInterfaceBase::ViewRefresh(void)
    {Send_ViewRefresh_ToView();}

void TDVecViewInterfaceBase::UseCursor(TDVecViewCursor eShape)
    {Send_UseCursor_ToView(eShape);}

double TDVecViewInterfaceBase::GetZoom(void) const
    {return(Send_GetZoom_ToView());}

void TDVecViewInterfaceBase::SetZoom(double nZoom, const POINT* pPoint)
    {Send_SetZoom_ToView(nZoom, pPoint);}

bool TDVecViewInterfaceBase::GetGridLock(void) const
    {return(Send_GetGridLock_ToView());}

bool TDVecViewInterfaceBase::IsRectVisible(TDMatRect MatRect) const
    {return(Send_IsRectVisible_ToView(MatRect));}

long TDVecViewInterfaceBase::GetMouseTolerance(void) const
    {return(Send_GetMouseTolerance_ToView());}

double TDVecViewInterfaceBase::GetMinimumDistance(void) const
    {return(Send_GetMinimumDistance_ToView());}

void TDVecViewInterfaceBase::ZoomInOutEvent(TDVOPZoomInOutExtVar* pExtVar) const
    {Send_ZoomInOutEvent_ToView(pExtVar);};
//---------------------------------------------------------------------------
// EOF
//---------------------------------------------------------------------------
