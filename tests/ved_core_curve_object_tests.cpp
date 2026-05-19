#include "vec_bezier_curve.h"
#include "vec_bspline.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

int expect(bool condition, const std::string& message) {
    return condition ? 0 : fail(message);
}

bool nearlyEqual(double a, double b) {
    return std::fabs(a - b) < 0.000001;
}

} // namespace

int main() {
    {
        TDVecBezierCurve bezier;
        bezier.AppendPoint({0.0, 0.0});
        bezier.AppendPoint({50.0, 100.0});
        bezier.AppendPoint({100.0, 0.0});
        bezier.SetShowPolygon(true);

        if (int result = expect(bezier.GetType() == VECOBJ_BZC, "bezier type mismatch")) {
            return result;
        }
        if (int result = expect(bezier.CountPoints() == 3, "bezier point count mismatch")) {
            return result;
        }

        const TDMatRect frame = bezier.GetFrame();
        if (int result = expect(nearlyEqual(frame.P1.x, 0.0) && nearlyEqual(frame.P1.y, 0.0) &&
                                    nearlyEqual(frame.P3.x, 100.0) && nearlyEqual(frame.P3.y, 100.0),
                                "bezier frame mismatch")) {
            return result;
        }

        bezier.ComputeCurve();
        if (int result = expect(bezier.HitTest({0.0, 0.0}, 0.01).IsHit(), "bezier hit test missed curve start")) {
            return result;
        }

        std::unique_ptr<TDVecObject> clone = bezier.Clone();
        if (int result = expect(clone && clone->GetType() == VECOBJ_BZC, "bezier clone type mismatch")) {
            return result;
        }

        bezier.MoveBy(10.0, -5.0);
        const TDMatPoint* first = bezier.GetPoint(0);
        if (int result = expect(first && nearlyEqual(first->x, 10.0) && nearlyEqual(first->y, -5.0),
                                "bezier move mismatch")) {
            return result;
        }
    }

    {
        TDVecBSPLine bspline;
        bspline.AppendPoint({0.0, 0.0});
        bspline.AppendPoint({50.0, 100.0});
        bspline.AppendPoint({100.0, 0.0});
        bspline.SetShowPolygon(true);
        bspline.GenerateKnot();

        if (int result = expect(bspline.GetType() == VECOBJ_BSPLINE, "bspline type mismatch")) {
            return result;
        }
        if (int result = expect(bspline.GetDegree() == 2 && bspline.GetResolution() == 50,
                                "bspline defaults mismatch")) {
            return result;
        }

        const TDMatRect frame = bspline.GetFrame();
        if (int result = expect(nearlyEqual(frame.P1.x, 0.0) && nearlyEqual(frame.P1.y, 0.0) &&
                                    nearlyEqual(frame.P3.x, 100.0) && nearlyEqual(frame.P3.y, 100.0),
                                "bspline frame mismatch")) {
            return result;
        }

        bspline.ComputeCurve();
        if (int result = expect(bspline.HitTest({0.0, 0.0}, 0.01).IsHit(), "bspline hit test missed curve start")) {
            return result;
        }

        std::unique_ptr<TDVecObject> clone = bspline.Clone();
        if (int result = expect(clone && clone->GetType() == VECOBJ_BSPLINE, "bspline clone type mismatch")) {
            return result;
        }

        bspline.MoveControle(1, 5.0, -5.0);
        const TDMatPoint* control = bspline.GetPoint(1);
        if (int result = expect(control && nearlyEqual(control->x, 55.0) && nearlyEqual(control->y, 95.0),
                                "bspline control move mismatch")) {
            return result;
        }
    }

    return 0;
}
