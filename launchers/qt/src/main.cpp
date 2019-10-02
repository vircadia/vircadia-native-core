#include <QtPlugin>

#include <qsharedmemory.h>

#include "LauncherWindow.h"
#include "Launcher.h"
#include "CommandlineOptions.h"
#include <iostream>
#include <string>
#include "Helper.h"

#ifdef Q_OS_WIN
#include "LauncherInstaller_windows.h"
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#elif defined(Q_OS_MACOS)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
#endif

Q_IMPORT_PLUGIN(QtQuick2Plugin);
Q_IMPORT_PLUGIN(QtQuickControls2Plugin);
Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin);

bool hasSuffix(const std::string& path, const std::string& suffix) {
    if (path.substr(path.find_last_of(".") + 1) == suffix) {
        return true;
    }

    return false;
}

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(resources);
#ifdef Q_OS_MAC
    if (isLauncherAlreadyRunning()) {
        return 0;
    }
    closeInterfaceIfRunning();
    if (argc == 3) {
        if (hasSuffix(argv[1], "app") && hasSuffix(argv[2], "app")) {
            swapLaunchers(argv[1], argv[2]);
        }
    }
#endif
    CommandlineOptions* options = CommandlineOptions::getInstance();
    options->parse(argc, argv);
#ifdef Q_OS_WIN
    LauncherInstaller launcherInstaller(argv[0]);
    if (options->contains("--restart") || launcherInstaller.runningOutsideOfInstallDir()) {
        launcherInstaller.install();
    } else if (options->contains("--uninstall")) {
        launcherInstaller.uninstall();
        return 0;
    }
#endif
    QString name { "High Fidelity" };
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName(name);

    Launcher launcher(argc, argv);

    return launcher.exec();

}
