//---------------------------------------------------------------------------
// HEADER:  Interface für dir View GUI-Komponente
//
// DESCR:
//
//
// HISTORY:
// Version   Datum       Autor       Wo/Was/Warum
//   1.0.0     28.08.00    tehrani     Erstellung
//---------------------------------------------------------------------------
#ifndef __VEC_VIEW_INTERFACE_H
#define __VEC_VIEW_INTERFACE_H

#include "vec_maindef.h"
#include "vec_graphic_engine.h"

class TDVOPZoomInOutExtVar;
//---------------------------------------------------------------------------
class TDVecViewInterfaceBase
{
public:
    TDVecViewInterfaceBase();
    virtual ~TDVecViewInterfaceBase() = 0;

    virtual void    Send_ViewRefresh_ToView(void) = 0;
    virtual void    Send_UseCursor_ToView(TDVecViewCursor eShape) = 0;
    virtual double  Send_GetZoom_ToView(void) const = 0;
    virtual void    Send_SetZoom_ToView(double nZoom, const POINT* pPoint) = 0;
    virtual bool    Send_GetGridLock_ToView(void) const = 0;
    virtual bool    Send_IsRectVisible_ToView(TDMatRect MatRect) const = 0;
    virtual long    Send_GetMouseTolerance_ToView(void) const = 0;
    virtual double  Send_GetMinimumDistance_ToView(void) const = 0;
    virtual void    Send_ZoomInOutEvent_ToView(TDVOPZoomInOutExtVar* pExtVar) const = 0;

    virtual void*   GetParent(void) = 0;

    TDGraphicEngine* GetGraphicEngine(void);
    void SetGraphicEngine(TDGraphicEngine* pGE);

    void    ViewRefresh(void);
    void    UseCursor(TDVecViewCursor eShape);
    double  GetZoom(void) const ;
    void    SetZoom(double nZoom, const POINT* pPoint);
    bool    GetGridLock(void) const;
    bool    IsRectVisible(TDMatRect MatRect) const;
    long    GetMouseTolerance(void) const;
    double  GetMinimumDistance(void) const;
    void    ZoomInOutEvent(TDVOPZoomInOutExtVar* pExtVar) const;

private:
    TDGraphicEngine* mpGE;
};


template <class T> class TDVecViewInterfaceTemplate : public TDVecViewInterfaceBase
{
public:
    TDVecViewInterfaceTemplate()
        : mpCallback_ViewRefresh(nullptr),
          mpCallback_UseCursor(nullptr),
          mpCallback_GetZoom(nullptr),
          mpCallback_SetZoom(nullptr),
          mpCallback_GetGridLock(nullptr),
          mpCallback_IsRectVisible(nullptr),
          mpCallback_GetMouseTolerance(nullptr),
          mpCallback_GetMinimumDistance(nullptr),
          mpCallback_ZoomInOutEvent(nullptr),
          mpObj(nullptr) {
    }

    inline void Register(T* pObj)
        {mpObj = pObj;}

    void* GetParent(void)
        {return(mpObj);}
    //------------------------------------------------------------
    void Send_ViewRefresh_ToView(void (T::* pCallback)(void))
        {mpCallback_ViewRefresh = pCallback;}

    void Send_ViewRefresh_ToView(void)
    {
        if(mpObj && mpCallback_ViewRefresh)
            (mpObj->*mpCallback_ViewRefresh)();
    }
    //------------------------------------------------------------
    void Send_UseCursor_ToView(void (T::* pCallback)(TDVecViewCursor eShape))
    {
        mpCallback_UseCursor = pCallback;
    }

    void Send_UseCursor_ToView(TDVecViewCursor eShape)
    {
        if(mpObj && mpCallback_UseCursor)
            (mpObj->*mpCallback_UseCursor)(eShape);
    }
    //------------------------------------------------------------

    void Send_GetZoom_ToView(double (T::* pCallback)(void)const)
        {mpCallback_GetZoom = pCallback;}

    double Send_GetZoom_ToView(void) const
    {
        if(mpObj && mpCallback_GetZoom)
            return((mpObj->*mpCallback_GetZoom)());
        else
            return(0);
    }
    //------------------------------------------------------------

    void Send_SetZoom_ToView(void (T::* pCallback)(double nZoom, const POINT* pPoint))
        {mpCallback_SetZoom = pCallback;}

    void Send_SetZoom_ToView(double nZoom, const POINT* pPoint)
    {
        if(mpObj && mpCallback_SetZoom)
            (mpObj->*mpCallback_SetZoom)(nZoom, pPoint);
    }
    //------------------------------------------------------------

    void Send_GetGridLock_ToView(bool (T::* pCallback)(void)const)
        {mpCallback_GetGridLock = pCallback;}

    bool Send_GetGridLock_ToView(void)const
    {
        if(mpObj && mpCallback_GetGridLock)
            return((mpObj->*mpCallback_GetGridLock)());
        else
            return(false);
    }
    //------------------------------------------------------------

    void Send_IsRectVisible_ToView(bool (T::* pCallback)(TDMatRect MatRect)const)
        {mpCallback_IsRectVisible = pCallback;}

    bool Send_IsRectVisible_ToView(TDMatRect MatRect) const
    {
        if(mpObj && mpCallback_IsRectVisible)
            return((mpObj->*mpCallback_IsRectVisible)(MatRect));
        else
            return(false);
    }
    //------------------------------------------------------------

    void Send_GetMouseTolerance_ToView(long (T::* pCallback)(void)const)
        {mpCallback_GetMouseTolerance = pCallback;}

    long Send_GetMouseTolerance_ToView(void) const
    {
        if(mpObj && mpCallback_GetMouseTolerance)
            return((mpObj->*mpCallback_GetMouseTolerance)());
        else
            return(0);
    }
    //------------------------------------------------------------

    void Send_GetMinimumDistance_ToView(double (T::* pCallback)(void)const)
        {mpCallback_GetMinimumDistance = pCallback;}

    double Send_GetMinimumDistance_ToView(void) const
    {
        if(mpObj && mpCallback_GetMinimumDistance)
            return((mpObj->*mpCallback_GetMinimumDistance)());
        else
            return(0.0);
    }
    //------------------------------------------------------------
    void Send_ZoomInOutEvent_ToView(void (T::* pCallback)(TDVOPZoomInOutExtVar* pExtVar)const)
        {mpCallback_ZoomInOutEvent = pCallback;}

    void Send_ZoomInOutEvent_ToView(TDVOPZoomInOutExtVar* pExtVar)const
    {
        if(mpObj && mpCallback_ZoomInOutEvent)
            (mpObj->*mpCallback_ZoomInOutEvent)(pExtVar);
    }
    //------------------------------------------------------------
private:
    void    (T::* mpCallback_ViewRefresh)(void);
    void    (T::* mpCallback_UseCursor)(TDVecViewCursor eShape);
    double  (T::* mpCallback_GetZoom)(void)const;
    void    (T::* mpCallback_SetZoom)(double nZoom, const POINT* pPoint);
    bool    (T::* mpCallback_GetGridLock)(void)const;
    bool    (T::* mpCallback_IsRectVisible)(TDMatRect MatRect)const;
    long    (T::* mpCallback_GetMouseTolerance)(void)const;
    double  (T::* mpCallback_GetMinimumDistance)(void)const;
    void    (T::* mpCallback_ZoomInOutEvent)(TDVOPZoomInOutExtVar* pExtVar)const;

    T* mpObj;
};

#endif
//---------------------------------------------------------------------------
// EOF
//---------------------------------------------------------------------------
