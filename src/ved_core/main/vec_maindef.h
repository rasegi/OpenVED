
#if !defined(__VEC_MAINDEF_H)
#define __VEC_MAINDEF_H

//---------------------------------------------------------------------------

enum TDViewOperationEnum
{
    VO_UNKNOWN                      ,        // ! unknown - Operation (not initializiert)

    VOC_LINE                        ,        // ! View-Operation-Create-Line
    VOC_CIRCLE_EDGE                 ,        // ! View-Operation-Create-Circle-Edge
    VOC_CIRCLE_MIDPOINT             ,        // ! View-Operation-Create-Circle-Midpoint
    VOC_CIRCLE_DIAMETER             ,        // ! View-Operation-Create-Circle-Diameter
    VOC_ELLIPSE_MIDPOINT            ,        // ! View-Operation-Create-Ellipse-Midpoint
    VOC_POLYGON_SMARTLINE           ,        // ! View-Operation-Create-Polygon-Smartline
    VOC_BEZIERCURVE_CONTROLPOINT    ,        // ! View-Operation-Create-BezierCurve-ControlPoint
    VOC_RECTANGLE_NOTROTATED        ,        // ! View-Operation-Create-Rectangle-NotRotated
    VOC_RECTANGLE_ROTATED           ,        // ! View-Operation-Create-Rectangle-Rotated
    VOC_BSPLINE_CONTROLPOINT        ,       // ! View-Operation-Create-BSPLine-ControlPoint
    VOC_POLYCURVE                   ,       // ! View-Operation-Create-PolyCurve
    VOC_VECTEXT                     ,       // ! View-Operation-Create-VecText
    VOC_VECFRAMETEXT_EMPTY          ,       // ! View-Operation-Create-VecFrameText-Empty
    VOC_ELLIPSE_ORTHOGONAL          ,       // ! View-Operation-Create-Ellipse-Orthogonal
    VOC_ROUND_RECTANGLE_NOTROTATED  ,       // ! View-Operation-Create-Round-Rectangle-NotRoatated
    VOC_CIRCLE_DIAGONAL             ,       // ! View-Operation-Create-Circle-Diagonal

    VOM_SELECT_OBJECT               ,      // ! View-Operation-Modify-Select-Object
    VOM_DELETE_OBJECT               ,      // ! View-Operation-Modify-Delete-Object
    VOM_MOVE_OBJECT                 ,      // ! View-Operation-Modify-Move-Object
    VOM_MOVE_NODE                   ,      // ! View-Operation-Modify-Move-Node
    VOM_MOVE_BSPLINE_CONTROLPOINT   ,      // ! View-Operation-Modify-Move-BezierControle
    VOM_MODIFY_CURVE_ATTRIBUTE      ,      // ! View-Operation-Modify-Curve-Attribute
    VOM_INSERT_NODE                 ,      // ! View-Operation-Modify-Insert-Node
    VOM_DELETE_NODE                 ,      // ! View-Operation-Modify-Delete-Node
    VOM_ROTATE_OBJECT_ACTIVEPOINT   ,      // ! View-Operation-Modify-Rotation-Object_AktivePoint
    VOM_SELECT_MOVE_NODE_OBJECT     ,      // ! View-Operation-Modify-Select-Move-Node-Object
    VOM_SCALE_OBJECT_3POINT         ,      // ! View-Operation-Modify-Scale-Object-3Point
    VOM_CHANGE_EDGE_ROUND_NODE      ,      // ! View-Operation-Modify-Change-EdgeRound-Node
    VOM_VECTEXT                     ,      // ! View-Operation-Modify-VecText
    VOM_INSERT_OBJECTS              ,      // ! View-Operation-Modify-Insert-Objects
    VOM_SELECTMOVE_OBJ_SCALE_FRAME  ,      // !View-Operation-Modify-SelectMove-Object-Scale-Frame

    VOP_ZOOM_INOUT                  ,     // ! View-Operation-Zoom-InOut
};

//---------------------------------------------------------------------------
enum TDViewOperationState
{
    OSTATE_UNKNOWN      = 0,
    OSTATE_STARTED      = 1,
    OSTATE_RUNNING      = 2,
    OSTATE_FINISHED     = 3,
    OSTATE_ABORTED      = 4,
    OSTATE_NEEDSELECTED = 5,
};
//---------------------------------------------------------------------------
enum TDOPVirtKey
{
    VIRTKEY_UNKNOWN     = 0,
    VIRTKEY_DELETE      = 1,
    VIRTKEY_CANCEL      = 2,
    VIRTKEY_CHANGE      = 3,
    VIRTKEY_PLUS        = 4,
    VIRTKEY_MINUS       = 5,

    VIRTKEY_MOVEDOWN    = 6,
    VIRTKEY_MOVEUP      = 7,
    VIRTKEY_MOVELEFT    = 8,
    VIRTKEY_MOVERIGHT   = 9,
};
//---------------------------------------------------------------------------
typedef unsigned long TDOPVirtKeyState;
static const TDOPVirtKeyState VKState_KEY_UNKNOWN  =   0x0000;
static const TDOPVirtKeyState VKState_KEY_SHIFT    =   0x0001;
static const TDOPVirtKeyState VKState_KEY_CTRL     =   0x0002;
static const TDOPVirtKeyState VKState_KEY_ALT      =   0x0004;
//---------------------------------------------------------------------------
enum TDOPVirtMouseButton
{
    VIRTMOUSEBTM_UNKNOWN    = 0,
    VIRTMOUSEBTM_LEFT       = 1,
    VIRTMOUSEBTM_MIDDLE     = 2,
    VIRTMOUSEBTM_RIGHT      = 3,
};

// Cursor Shapes speziell für VecView
enum TDVecViewCursor
{
    VECVIEW_CURSOR_DEFAULT  = 0,

    VECVIEW_ZOOM,
    VECVIEW_SELECT_OBJECT,
    VECVIEW_PLUS,
    VECVIEW_DOCK,
    VECVIEW_NO,
    VECVIEW_SIMPLE_CROSS,
    VECVIEW_CROSS_PLUS,
    VECVIEW_CROSS_MINUS,

    VECVIEW_CREATE_LINE_1,
    VECVIEW_CREATE_LINE_2,
    VECVIEW_CREATE_CIRCLE_1,
    VECVIEW_CREATE_CIRCLE_2,
    VECVIEW_CREATE_ELLIPSE_ROTATED_1,
    VECVIEW_CREATE_ELLIPSE_ROTATED_2,
    VECVIEW_CREATE_ELLIPSE_NOTROTATED_1,
    VECVIEW_CREATE_ELLIPSE_NOTROTATED_2,
    VECVIEW_CREATE_POLYGON_SMARTLINE_1,
    VECVIEW_CREATE_POLYGON_SMARTLINE_2,
    VECVIEW_CREATE_POLYCURVE_SMARTLINE_1,
    VECVIEW_CREATE_POLYCURVE_SMARTLINE_2,
    VECVIEW_CREATE_RECTANGLE_ROTATED_1,
    VECVIEW_CREATE_RECTANGLE_ROTATED_2,
    VECVIEW_CREATE_RECTANGLE_NOTROTATED_1,
    VECVIEW_CREATE_RECTANGLE_NOTROTATED_2,
    VECVIEW_CREATE_BSPLINE_1,
    VECVIEW_CREATE_BSPLINE_2,
    VECVIEW_CREATE_VECTEXT_1,
    VECVIEW_CREATE_VECTEXT_2,
    VECVIEW_DELETE_OBJECT,

    VECVIEW_CURSOR_MAX
};

// ! Object Typen
enum TDVecObjectType
{
    VECOBJ_UNKNOWN          = 0,
    VECOBJ_LIN              = 1,    // ! Linie
    VECOBJ_ELL              = 2,    // ! Ellipse
    VECOBJ_POL              = 3,    // ! Polygon-Zuge
    VECOBJ_POLAREA          = 4,    // ! Polygon-Fläche
    VECOBJ_BSPLINE          = 5,    // ! BSPLine-Curve
    VECOBJ_BZC              = 6,    // ! Bezier-Curve
    VECOBJ_TEX              = 7,    // ! Text
    VECOBJ_FRAMETEXT        = 8,    // ! Text-Rahmen
    VECOBJ_GRP              = 9,    // ! Gruppe
    VECOBJ_POLYCURVE        = 10,   // ! PolyCurve
    VECOBJ_POLYCURVEAREA    = 11,   // ! PolyCurveArea
};
// von Graphic-Engine
enum    TDNodeType
{
    NODE_UNKNOWN        = 0 ,
    NODE_NORMAL         = 1 ,
    NODE_PROPORTIONAL   = 3 ,
    NODE_CONTROL        = 5 ,

    NODE_POLY_START     = 7 ,
    NODE_POLY_END       = 9 ,
    NODE_MIDPOINT       = 11,
};
// RGB-Farbe (Windows)
typedef unsigned long TDRgbColor;

class TDVecObject;

#endif
