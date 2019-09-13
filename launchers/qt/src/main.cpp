#include <QtPlugin>

#include "LauncherWindow.h"
#include "Launcher.h"

//Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QtQuick2Plugin);
Q_IMPORT_PLUGIN(QtQuickControls2Plugin);
Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin);

int main(int argc, char *argv[]) {
    QString name { "High Fidelity" };
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName(name);

    Launcher launcher(argc, argv);

    return launcher.exec();

}
