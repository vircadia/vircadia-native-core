#pragma once

#include <QDir>
class LauncherInstaller {
public:
    LauncherInstaller(const QString& applicationFilePath);
    ~LauncherInstaller() = default;

    void install();
    void uninstall();
private:
    bool runningOutsideOfInstallDir();
    QDir _launcherInstallDir;
    QString _launcherRunningFilePath;
    QString _launcherRunningDirPath;
};
