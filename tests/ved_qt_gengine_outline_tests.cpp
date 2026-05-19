#include "vec_graphic_engine_qt.h"

#include <QGuiApplication>
#include <QColor>
#include <QImage>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

double distancePointToSegment(double px, double py, double x1, double y1, double x2, double y2) {
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= 0.0) {
        return std::hypot(px - x1, py - y1);
    }
    const double t = std::clamp(((px - x1) * dx + (py - y1) * dy) / lengthSquared, 0.0, 1.0);
    const double nearestX = x1 + t * dx;
    const double nearestY = y1 + t * dy;
    return std::hypot(px - nearestX, py - nearestY);
}

} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);

    QImage image(220, 220, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(250, 250, 248));

    QPainter painter(&image);
    TDGraphicEngineQt graphicEngine;
    graphicEngine.SetDeviceMetrics(image.width(), image.height(), 96.0, 96.0);
    graphicEngine.SetViewRange(0.0, 0.0, 220.0, 220.0);
    graphicEngine.SetPainter(&painter);
    graphicEngine.SetDrawColor(0x00000000);

    const TDMatLine line{20.0, 20.0, 180.0, 80.0};
    graphicEngine.DrawLine(&line, true);
    graphicEngine.SetPainter(nullptr);
    painter.end();

    int changedPixels = 0;
    int farPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) == QColor(250, 250, 248)) {
                continue;
            }
            ++changedPixels;
            if (distancePointToSegment(x, y, 20.0, 20.0, 180.0, 80.0) > 2.0) {
                ++farPixels;
            }
        }
    }

    if (changedPixels == 0) {
        return fail("outline line did not draw any pixels");
    }
    if (farPixels != 0) {
        return fail("outline line changed pixels away from the line: " + std::to_string(farPixels));
    }

    return 0;
}
