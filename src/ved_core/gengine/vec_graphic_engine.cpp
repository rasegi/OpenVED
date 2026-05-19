//---------------------------------------------------------------------------
// MODULE: Graphic-Engine
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

#define __VEC_GRAPHIC_ENGINE_CPP
#include "vec_graphic_engine.h"


//---------------------------------------------------------------------------
// DESCR:
//  Konstruktor. Erzeugt eine Graphic Engine.
// INPUT:
//  hDC     -   Device Context für Grafikausgabe
//  hWnd    -   assoziiertes Fenster
//---------------------------------------------------------------------------
TDGraphicEngine::TDGraphicEngine()
{}
//---------------------------------------------------------------------------
// DESCR:
//  Virtual-Destruktor.
//---------------------------------------------------------------------------
TDGraphicEngine::~TDGraphicEngine()
{}

//---------------------------------------------------------------------------
// EOF
//---------------------------------------------------------------------------

