#include "PathUtils.h"

#include "Helper.h"

#include <QDir>
#include <QDebug>
#include <QStandardPaths>

QUrl PathUtils::resourcePath(const QString& source) {
    QString filePath = RESOURCE_PREFIX_URL + source;
#ifdef HIFI_USE_LOCAL_FILE
    return QUrl::fromLocalFile(filePath);
#else
    return QUrl(filePath);
#endif
}

QString PathUtils::fontPath(const QString& fontName) {
#ifdef HIFI_USE_LOCAL_FILE
    return resourcePath("/fonts/" + fontName).toString(QUrl::PreferLocalFile);
#else
    return ":/fonts/" + fontName;
#endif
}

QDir PathUtils::getLauncherDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QDir PathUtils::getApplicationsDirectory() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)).absoluteFilePath("HQ");
}

// The client directory is where interface is installed to.
QDir PathUtils::getClientDirectory() {
    return getLauncherDirectory().filePath("client");
}

QDir PathUtils::getLogsDirectory() {
    return getLauncherDirectory().filePath("logs");
}

// The download directory is used to store files downloaded during installation.
QDir PathUtils::getDownloadDirectory() {
    return getLauncherDirectory().filePath("downloads");
}

// The content cache path is the directory interface uses for caching data.
// It is pre-populated on startup with domain content.
QString PathUtils::getContentCachePath() {
    return getLauncherDirectory().filePath("contentcache");
}

// The path to the interface binary.
QString PathUtils::getClientExecutablePath() {
    QDir clientDirectory = getClientDirectory();
#if defined(Q_OS_WIN)
    return clientDirectory.absoluteFilePath("interface.exe");
#elif defined(Q_OS_MACOS)
    return clientDirectory.absoluteFilePath("interface.app/Contents/MacOS/interface");
#endif
}

// The path to the config.json file that the launcher uses to store information like
// the last user that logged in.
QString PathUtils::getConfigFilePath() {
    return getClientDirectory().absoluteFilePath("config.json");
}

// The path to the launcher binary.
QString PathUtils::getLauncherFilePath() {
#if defined(Q_OS_WIN)
    return getLauncherDirectory().absoluteFilePath("HQ Launcher.exe");
#elif defined(Q_OS_MACOS)
    return getBundlePath() + "/Contents/MacOS/HQ Launcher";
#endif
}
