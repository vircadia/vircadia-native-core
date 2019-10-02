#include "LauncherInstaller_windows.h"
#include "Helper.h"

#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

LauncherInstaller::LauncherInstaller(const QString& applicationFilePath) {
    _launcherInstallDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/HQ";
    _launcherApplicationsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/HQ";
    qDebug() << "Launcher install dir: " << _launcherInstallDir.absolutePath();
    qDebug() << "Launcher Application dir: " << _launcherApplicationsDir.absolutePath();
    _launcherInstallDir.mkpath(_launcherInstallDir.absolutePath());
    _launcherApplicationsDir.mkpath(_launcherApplicationsDir.absolutePath());
    QFileInfo fileInfo(applicationFilePath);
    _launcherRunningFilePath = fileInfo.absoluteFilePath();
    _launcherRunningDirPath = fileInfo.absoluteDir().absolutePath();
    qDebug() << "Launcher running file path: " << _launcherRunningFilePath;
    qDebug() << "Launcher running dir path: " << _launcherRunningDirPath;
}

bool LauncherInstaller::runningOutsideOfInstallDir() {
    return (QString::compare(_launcherInstallDir.absolutePath(), _launcherRunningDirPath) != 0);
}

void LauncherInstaller::install() {
    //qDebug() << "Is install dir empty: " << _launcherInstallDir.isEmpty();
    if (runningOutsideOfInstallDir()) {
        qDebug() << "Installing HQ Launcher....";
        QString oldLauncherPath = _launcherInstallDir.absolutePath() + "/HQ Launcher.exe";

        if (QFile::exists(oldLauncherPath)) {
            bool didRemove = QFile::remove(oldLauncherPath);
            qDebug() << "did remove file: " << didRemove;
        }
        bool success = QFile::copy(_launcherRunningFilePath, oldLauncherPath);
        if (success) {
            qDebug() << "successful";
        } else {
            qDebug() << "not successful";
        }

        qDebug() << "LauncherInstaller: create uninstall link";
        QString uninstallLinkPath = _launcherInstallDir.absolutePath() + "/Uninstall HQ.lnk";
        if (QFile::exists(uninstallLinkPath)) {
            QFile::remove(uninstallLinkPath);
        }



        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        QString applicationPath = _launcherApplicationsDir.absolutePath();

        QString appStartLinkPath = applicationPath + "/HQ.lnk";
        QString uninstallAppStartLinkPath = applicationPath + "/Uninstall HQ.lnk";
        QString desktopAppLinkPath = desktopPath  + "/HQ.lnk";


        createSymbolicLink((LPCSTR)oldLauncherPath.toStdString().c_str(), (LPCSTR)uninstallLinkPath.toStdString().c_str(),
                           (LPCSTR)("Click to Uninstall HQ"), (LPCSTR)("--uninstall"));

        createSymbolicLink((LPCSTR)oldLauncherPath.toStdString().c_str(), (LPCSTR)uninstallAppStartLinkPath.toStdString().c_str(),
                           (LPCSTR)("Click to Uninstall HQ"), (LPCSTR)("--uninstall"));

        createSymbolicLink((LPCSTR)oldLauncherPath.toStdString().c_str(), (LPCSTR)desktopAppLinkPath.toStdString().c_str(),
                           (LPCSTR)("Click to Setup and Launch HQ"));

        createSymbolicLink((LPCSTR)oldLauncherPath.toStdString().c_str(), (LPCSTR)appStartLinkPath.toStdString().c_str(),
                           (LPCSTR)("Click to Setup and Launch HQ"));

        createApplicationRegistryKeys();
    } else {
        qDebug() << "FAILED!!!!!!!";
    }
}

void LauncherInstaller::uninstall() {
    qDebug() << "Uninstall Launcher";
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString applicationPath = _launcherApplicationsDir.absolutePath();

    QString uninstallLinkPath = _launcherInstallDir.absolutePath() + "/Uninstall HQ.lnk";
    if (QFile::exists(uninstallLinkPath)) {
        QFile::remove(uninstallLinkPath);
    }

    QString appStartLinkPath = applicationPath + "/HQ.lnk";
    if (QFile::exists(appStartLinkPath)) {
        QFile::remove(appStartLinkPath);
    }

    QString uninstallAppStartLinkPath = applicationPath + "/Uninstall HQ.lnk";
    if (QFile::exists(uninstallAppStartLinkPath)) {
        QFile::remove(uninstallAppStartLinkPath);
    }

    QString desktopAppLinkPath = desktopPath  + "/HQ.lnk";
    if (QFile::exists(desktopAppLinkPath)) {
        QFile::remove(desktopAppLinkPath);
    }

    deleteApplicationRegistryKeys();
}


void LauncherInstaller::createApplicationRegistryKeys() {
    const std::string REGISTRY_PATH = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HQ";
    bool success = insertRegistryKey(REGISTRY_PATH, "DisplayName", "HQ");
    std::string installPath = _launcherInstallDir.absolutePath().toStdString();
    success = insertRegistryKey(REGISTRY_PATH, "InstallLocation", installPath);
    std::string applicationExe = installPath + "/HQ Launcher.exe";
    std::string uninstallPath = '"' + applicationExe + '"' + " --uninstall";
    success = insertRegistryKey(REGISTRY_PATH, "UninstallString", uninstallPath);
    success = insertRegistryKey(REGISTRY_PATH, "DisplayVersion", "DEV");
    success = insertRegistryKey(REGISTRY_PATH, "DisplayIcon", applicationExe);
    success = insertRegistryKey(REGISTRY_PATH, "Publisher", "High Fidelity");

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::stringstream date;
    date << std::put_time(std::localtime(&now), "%Y-%m-%d") ;
    success = insertRegistryKey(REGISTRY_PATH, "InstallDate", date.str());
    //success = LauncherUtils::insertRegistryKey(REGISTRY_PATH, "EstimatedSize", (DWORD)size);
    success = insertRegistryKey(REGISTRY_PATH, "NoModify", (DWORD)1);
    success = insertRegistryKey(REGISTRY_PATH, "NoRepair", (DWORD)1);

    qDebug() << "Did succcessfully insertRegistyKeys: " << success;
}

void LauncherInstaller::deleteApplicationRegistryKeys() {
    const std::string regPath= "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HQ";
    if (!deleteRegistryKey(regPath.c_str())) {
        qDebug() << "Failed to delete registryKeys";
    }
}
