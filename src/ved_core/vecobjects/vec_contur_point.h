#pragma once

enum TDConturPointType {
    CPT_PRIME_LINE = 1,
    CPT_PRIME_QSPLINE = 2
};

struct TDMatConturPoint {
    double x = 0.0;
    double y = 0.0;
    TDConturPointType eType = CPT_PRIME_LINE;
    bool bJoint = false;
};
