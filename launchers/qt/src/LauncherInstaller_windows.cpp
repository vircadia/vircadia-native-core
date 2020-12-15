#include "LauncherInstaller_windows.h"

#include "CommandlineOptions.h"
#include "Helper.h"
#include "PathUtils.h"

#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QTime>


void timeDelay() {
    static const int ONE_SECOND = 1;
    QTime delayTime = QTime::currentTime().addSecs(ONE_SECOND);
    while (QTime::currentTime() < delayTime) {
        qDebug() << "Delaying time";
    }
}
LauncherInstaller::LauncherInstaller() {
    _launcherInstallDir = PathUtils::getLauncherDirectory();
    _launcherApplicationsDir = PathUtils::getApplicationsDirectory();
    qDebug() << "Launcher install dir: " << _launcherInstallDir.absolutePath();
    qDebug() << "Launcher Application dir: " << _launcherApplicationsDir.absolutePath();

    _launcherInstallDir.mkpath(_launcherInstallDir.absolutePath());
    _launcherApplicationsDir.mkpath(_launcherApplicationsDir.absolutePath());
    char appPath[MAX_PATH];
    GetModuleFileNameA(NULL, appPath, MAX_PATH);
    QString applicationRunningPath = appPath;
    QFileInfo fileInfo(applicationRunningPath);
    _launcherRunningFilePath = fileInfo.absoluteFilePath();
    _launcherRunningDirPath = fileInfo.absoluteDir().absolutePath();
    qDebug() << "Launcher running file path: " << _launcherRunningFilePath;
    qDebug() << "Launcher running dir path: " << _launcherRunningDirPath;
}

bool LauncherInstaller::runningOutsideOfInstallDir() {
    return (QString::compare(_launcherInstallDir.absolutePath(), _launcherRunningDirPath) != 0);
}

void LauncherInstaller::install() {
    if (runningOutsideOfInstallDir()) {
        qDebug() << "Installing HQ Launcher....";
        uninstallOldLauncher();
        QString oldLauncherPath = PathUtils::getLauncherFilePath();

        if (QFile::exists(oldLauncherPath)) {
            bool didRemove = QFile::remove(oldLauncherPath);
            qDebug() << "did remove file: " << didRemove;
        }
        qDebug() << "Current launcher location: " << _launcherRunningFilePath;
        bool success = QFile::copy(_launcherRunningFilePath, oldLauncherPath);
        if (success) {
            qDebug() << "Launcher installed: " << oldLauncherPath;
        } else {
            qDebug() << "Failed to install: " << oldLauncherPath;
        }

        deleteShortcuts();
        deleteApplicationRegistryKeys();
        createShortcuts();
        createApplicationRegistryKeys();
    } else {
        qDebug() << "Failed to install HQ Launcher";
    }
}

void LauncherInstaller::createShortcuts() {
    QString launcherPath = PathUtils::getLauncherFilePath();

    QString uninstallLinkPath = _launcherInstallDir.absoluteFilePath("Uninstall HQ Launcher.lnk");

    QDir desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    QString appStartLinkPath = _launcherApplicationsDir.absoluteFilePath("HQ Launcher.lnk");
    QString uninstallAppStartLinkPath = _launcherApplicationsDir.absoluteFilePath("Uninstall HQ Launcher.lnk");
    QString desktopAppLinkPath = desktopDir.absoluteFilePath("HQ Launcher.lnk");


    createSymbolicLink((LPCSTR)launcherPath.toStdString().c_str(), (LPCSTR)uninstallLinkPath.toStdString().c_str(),
                       (LPCSTR)("Click to Uninstall HQ Launcher"), (LPCSTR)("--uninstall"));

    createSymbolicLink((LPCSTR)launcherPath.toStdString().c_str(), (LPCSTR)uninstallAppStartLinkPath.toStdString().c_str(),
                       (LPCSTR)("Click to Uninstall HQ Launcher"), (LPCSTR)("--uninstall"));

    createSymbolicLink((LPCSTR)launcherPath.toStdString().c_str(), (LPCSTR)desktopAppLinkPath.toStdString().c_str(),
                       (LPCSTR)("Click to Setup and Launch HQ Launcher"));

    createSymbolicLink((LPCSTR)launcherPath.toStdString().c_str(), (LPCSTR)appStartLinkPath.toStdString().c_str(),
                           (LPCSTR)("Click to Setup and Launch HQ Launcher"));
}

QString randomQtString() {
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   const int randomStringLength = 5;
   auto now = std::chrono::system_clock::now();
   auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
   qsrand(duration.count());

   QString randomString;
   for(int i = 0; i < randomStringLength; i++)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}

void LauncherInstaller::uninstall() {
    qDebug() << "Uninstall Launcher";
    deleteShortcuts();
    CommandlineOptions* options = CommandlineOptions::getInstance();
    if (!options->contains("--resumeUninstall")) {
        QDir tmpDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString destination = tmpDirectory.absolutePath() + "/" + randomQtString() + ".exe";
        qDebug() << "temp file destination: " << destination;
        bool copied = QFile::copy(_launcherRunningFilePath, destination);

        if (copied) {
            QString params = "\"" + _launcherRunningFilePath + "\"" +  " --resumeUninstall";
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            BOOL success = CreateProcess(
                destination.toUtf8().data(),
                params.toUtf8().data(),
                nullptr,                // Process handle not inheritable
                nullptr,                // Thread handle not inheritable
                FALSE,                  // Set handle inheritance to FALSE
                CREATE_NEW_CONSOLE,     // Opens file in a separate console
                nullptr,                // Use parent's environment block
                nullptr,                // Use parent's starting directory
                &si,                    // Pointer to STARTUPINFO structure
                &pi                     // Pointer to PROCESS_INFORMATION structure
            );
        } else {
            qDebug() << "Failed to complete uninstall launcher";
        }
        return;
    } else {

        bool deletedHQLauncherExe = deleteHQLauncherExecutable();
        if (deletedHQLauncherExe) {
            qDebug() << "Deleteing registry keys";
            deleteApplicationRegistryKeys();
        }
    }
}

bool LauncherInstaller::deleteHQLauncherExecutable() {
    static const int MAX_DELETE_ATTEMPTS = 3;

    int deleteAttempts = 0;
    bool fileRemoved = false;
    QString launcherPath = _launcherInstallDir.absoluteFilePath("HQ Launcher.exe");
    while (deleteAttempts < MAX_DELETE_ATTEMPTS) {
        fileRemoved = QFile::remove(launcherPath);
        if (fileRemoved) {
            break;
        }

        timeDelay();
        deleteAttempts++;
    }

    qDebug() << "Successfully removed " << launcherPath << ": " << fileRemoved;

    return fileRemoved;
}

void LauncherInstaller::deleteShortcuts() {
    QDir desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString applicationPath = _launcherApplicationsDir.absolutePath();

    QString uninstallLinkPath = _launcherInstallDir.absoluteFilePath("Uninstall HQ Launcher.lnk");
    if (QFile::exists(uninstallLinkPath)) {
        QFile::remove(uninstallLinkPath);
    }

    QString appStartLinkPath = _launcherApplicationsDir.absoluteFilePath("HQ Launcher.lnk");
    if (QFile::exists(appStartLinkPath)) {
        QFile::remove(appStartLinkPath);
    }

    QString uninstallAppStartLinkPath = _launcherApplicationsDir.absoluteFilePath("Uninstall HQ Launcher.lnk");
    if (QFile::exists(uninstallAppStartLinkPath)) {
        QFile::remove(uninstallAppStartLinkPath);
    }

    QString desktopAppLinkPath = desktopDir.absoluteFilePath("HQ Launcher.lnk");
    if (QFile::exists(desktopAppLinkPath)) {
        QFile::remove(desktopAppLinkPath);
    }
}

void LauncherInstaller::uninstallOldLauncher() {
    QDir localAppDir = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation).value(0) + "/../../HQ";
    QDir startAppDir = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).value(0) + "/HQ";
    QDir desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    qDebug() << localAppDir.absolutePath();
    qDebug() << startAppDir.absolutePath();
    QString desktopAppLinkPath = desktopDir.absoluteFilePath("HQ Launcher.lnk");
    if (QFile::exists(desktopAppLinkPath)) {
        QFile::remove(desktopAppLinkPath);
    }

    QString uninstallLinkPath = localAppDir.absoluteFilePath("Uninstall HQ.lnk");
    if (QFile::exists(uninstallLinkPath)) {
        QFile::remove(uninstallLinkPath);
    }

    QString applicationPath = localAppDir.absoluteFilePath("HQ Launcher.exe");
    if (QFile::exists(applicationPath)) {
        QFile::remove(applicationPath);
    }

    QString appStartLinkPath = startAppDir.absoluteFilePath("HQ Launcher.lnk");
    if (QFile::exists(appStartLinkPath)) {
        QFile::remove(appStartLinkPath);
    }

    QString uninstallAppStartLinkPath = startAppDir.absoluteFilePath("Uninstall HQ.lnk");
    if (QFile::exists(uninstallAppStartLinkPath)) {
        QFile::remove(uninstallAppStartLinkPath);
    }
}



void LauncherInstaller::createApplicationRegistryKeys() {
    const std::string REGISTRY_PATH = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HQ";
    bool success = insertRegistryKey(REGISTRY_PATH, "DisplayName", "HQ Launcher");
    std::string installPath = _launcherInstallDir.absolutePath().replace("/", "\\").toStdString();
    success = insertRegistryKey(REGISTRY_PATH, "InstallLocation", installPath);
    std::string applicationExe = installPath + "\\HQ Launcher.exe";
    std::string uninstallPath = applicationExe + " --uninstall";
    qDebug() << QString::fromStdString(applicationExe);
    qDebug() << QString::fromStdString(uninstallPath);
    success = insertRegistryKey(REGISTRY_PATH, "UninstallString", uninstallPath);
    success = insertRegistryKey(REGISTRY_PATH, "DisplayVersion", std::string(LAUNCHER_BUILD_VERSION));
    success = insertRegistryKey(REGISTRY_PATH, "DisplayIcon", applicationExe);
    success = insertRegistryKey(REGISTRY_PATH, "Publisher", "Vircadia");

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::stringstream date;
    date << std::put_time(std::localtime(&now), "%Y-%m-%d") ;
    success = insertRegistryKey(REGISTRY_PATH, "InstallDate", date.str());
    success = insertRegistryKey(REGISTRY_PATH, "EstimatedSize", (DWORD)14181);
    success = insertRegistryKey(REGISTRY_PATH, "NoModify", (DWORD)1);
    success = insertRegistryKey(REGISTRY_PATH, "NoRepair", (DWORD)1);

    qDebug() << "Did succcessfully insertRegistyKeys: " << success;
}

void LauncherInstaller::deleteApplicationRegistryKeys() {
    const std::string regPath= "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HQ";
    bool success = deleteRegistryKey(regPath.c_str());
    qDebug() << "Did delete Application Registry Keys: " << success;
}
