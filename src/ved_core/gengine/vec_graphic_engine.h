//---------------------------------------------------------------------------
// HEADER: Graphic-Engine
//
// DESCR:
//  TDGraphicEngine ist eine Klasse, die eine geräte- und (in Zukunft auch)
//  plattformunabhängige Schnittstelle für Grafikausgaben zur Verfügung
//  stellt. Der Einsatz ist momentan auf den Vektoreditor von WPS beschränkt.
//
// HISTORY:
// Version   Datum       Autor       Wo/Was/Warum
// V1.0      23.10.99    tehrani     Ersterstellung
//---------------------------------------------------------------------------

#ifndef __VEC_GRAPHIC_ENGINE_H
#define __VEC_GRAPHIC_ENGINE_H

#include "vec_math_base.h"
#include "vec_maindef.h"

// Graphic Engine
class TDGraphicEngine
{
public:
    TDGraphicEngine();
    virtual ~TDGraphicEngine() = 0;

    // Zeichnen
    virtual void    DrawEllipse(const TDMatEllipse* pParams, bool bOutLine) = 0;
    virtual void    DrawLine(const TDMatLine* pParams, bool bOutLine) = 0;
    virtual void    DrawRectangle(const TDMatRect* pParams, bool bOutLine) = 0;
    virtual void    DrawPolygon(const TDMatPointsArray* pParams, bool bOutLine) = 0;
    virtual void    DrawLineOutLine(const TDMatLine* pParams) = 0;
    virtual void    DrawConstructPolygon(const TDMatPointsArray* pParams) = 0;
    virtual void    DrawBoxOutLine(TDMatPoint MatPoint1, TDMatPoint MatPoint2) = 0;
    virtual void    DrawRulers(long nDist, long nSubDiv, long nResLimit) {};

    virtual void    DrawNode(double x, double y , TDNodeType eNodeType , bool bLock) = 0;
    virtual void    DrawFrame(const TDMatRect* pParams) = 0;
    // Real To Screen
    virtual long    RealToXPos(double x) const = 0;
    virtual long    RealToYPos(double y) const = 0;
    virtual long    RealToXPos(long x) const = 0;
    virtual long    RealToYPos(long y) const = 0;
    virtual long    RealToXVal(double x) const = 0;
    virtual long    RealToYVal(double y) const = 0;
    virtual long    RealToXVal(long x) const = 0;
    virtual long    RealToYVal(long y) const = 0;

    // Screen To Real
    virtual double  ScreenToXPos(long x) const = 0;
    virtual double  ScreenToYPos(long y) const = 0;
    virtual double  ScreenToXVal(long x) const = 0;
    virtual double  ScreenToYVal(long y) const = 0;

    // Farben
    virtual void        SetDrawColor(TDRgbColor nColor) = 0;
    virtual TDRgbColor  GetDrawColor() const = 0;

    // Nodes
    virtual int         GetNodeSize() const = 0;
    virtual void        SetNodeSize(int nSize) = 0;

    // als Schnitstelle ... (fals notwendig in abgeleitete Klassen mussen überschreiben werden)
    virtual bool    IsRectVisible(TDMatRect MatRect) const {return(false);};
    virtual long    GetMouseTolerance() const {return(0);};
    virtual double  GetMinimumDistance() const {return(0.0);};


private:

};
#endif
//---------------------------------------------------------------------------
// EOF
//---------------------------------------------------------------------------
