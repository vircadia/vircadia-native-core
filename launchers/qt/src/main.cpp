#include <QtPlugin>

#include "LauncherWindow.h"
#include "Launcher.h"
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



bool hasSuffix(const std::string path, const std::string suffix) {
    if (path.substr(path.find_last_of(".") + 1) == suffix) {
        return true;
    }

    return false;
}

bool containsOption(int argc, char* argv[], const std::string& option) {
    for (int index = 0; index < argc; index++) {
        if (option.compare(argv[index]) == 0) {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[]) {

    //std::cout << "Launcher version: " << LAUNCHER_BUILD_VERSION;
#ifdef Q_OS_MAC
    // auto updater
    if (argc == 3) {
        if (hasSuffix(argv[1], "app") && hasSuffix(argv[2], "app")) {
            std::cout << "swapping launcher \n";
            swapLaunchers(argv[1], argv[2]);
        } else {
            std::cout << "not swapping launcher \n";
        }
    }
#elif defined(Q_OS_WIN)
    // try-install

    if (containsOption(argc, argv, "--restart")) {
        LauncherInstaller launcherInstaller(argv[0]);
        launcherInstaller.install();
    }
#endif
    QString name { "High Fidelity" };
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName(name);

    Launcher launcher(argc, argv);

    return launcher.exec();

}
