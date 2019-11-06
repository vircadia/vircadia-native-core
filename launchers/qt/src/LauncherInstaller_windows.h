#pragma once

#include <QDir>
class LauncherInstaller {
public:
    LauncherInstaller();
    ~LauncherInstaller() = default;

    void install();
    void uninstall();
    bool runningOutsideOfInstallDir();
private:
    void createShortcuts();
    void uninstallOldLauncher();
    void createApplicationRegistryKeys();
    void deleteShortcuts();
    void deleteApplicationRegistryKeys();
    bool deleteHQLauncherExecutable();

    QDir _launcherInstallDir;
    QDir _launcherApplicationsDir;
    QString _launcherRunningFilePath;
    QString _launcherRunningDirPath;
};
