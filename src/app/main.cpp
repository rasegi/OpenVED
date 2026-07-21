#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("VED"));
    QCoreApplication::setApplicationName(QStringLiteral("VED Qt Port"));
    app.setWindowIcon(QIcon(QStringLiteral(":/ved/openved.png")));

    MainWindow window;
    window.show();

    return QApplication::exec();
}
