#include <QtPlugin>

#include <qsharedmemory.h>

#include "LauncherWindow.h"
#include "Launcher.h"
#include "CommandlineOptions.h"
#include <iostream>
#include <string>
#include <QProcess>
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
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("Vircadia");
    QCoreApplication::setApplicationName("HQ Launcher");

    Q_INIT_RESOURCE(resources);
    cleanLogFile();
    qInstallMessageHandler(messageHandler);
    CommandlineOptions* options = CommandlineOptions::getInstance();
    options->parse(argc, argv);
    bool didUpdate = false;

#ifdef Q_OS_MAC
    if (isLauncherAlreadyRunning()) {
        return 0;
    }
    closeInterfaceIfRunning();
    if (argc == 3) {
        if (hasSuffix(argv[1], "app") && hasSuffix(argv[2], "app")) {
            bool success = swapLaunchers(argv[1], argv[2]);
            qDebug() << "Successfully installed Launcher: " << success;
            options->append("--noUpdate");
            didUpdate = true;
        }
    }
#endif

    if (options->contains("--version")) {
        std::cout << LAUNCHER_BUILD_VERSION << std::endl;
        return 0;
    }

#ifdef Q_OS_WIN
    LauncherInstaller launcherInstaller;
    if (options->contains("--uninstall") || options->contains("--resumeUninstall")) {
        launcherInstaller.uninstall();
        return 0;
    } else if (options->contains("--restart") || launcherInstaller.runningOutsideOfInstallDir()) {
        launcherInstaller.install();
    }

    int interfacePID = -1;
    if (isProcessRunning("interface.exe", interfacePID)) {
        shutdownProcess(interfacePID, 0);
    }
#endif

    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    if (processEnvironment.contains("HQ_LAUNCHER_BUILD_VERSION")) {
        if (didUpdate || options->contains("--restart")) {
            options->append("--noUpdate");
        }
    }

    Launcher launcher(argc, argv);
    return launcher.exec();
}
