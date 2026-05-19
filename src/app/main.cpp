#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("VED"));
    QCoreApplication::setApplicationName(QStringLiteral("VED Qt Port"));

    MainWindow window;
    window.show();

    return QApplication::exec();
}
