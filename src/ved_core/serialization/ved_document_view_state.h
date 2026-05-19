#pragma once

#include "vec_math_base.h"

struct VEDDocumentViewState {
    bool present = false;
    double zoom = 1.0;
    TDMatPoint center{0.0, 0.0};
    bool showGrid = false;
    bool gridLock = false;
    bool showRulers = false;
};
