#pragma once

#include <QDir>
#include <QObject>
#include <QString>
#include <QFile>
#include <QUrl>

class PathUtils : public QObject {
    Q_OBJECT
public:
    PathUtils() = default;
    ~PathUtils() = default;
    Q_INVOKABLE static QUrl resourcePath(const QString& source);

    static QString fontPath(const QString& fontName);

    static QDir getLauncherDirectory();
    static QDir getApplicationsDirectory();
    static QDir getDownloadDirectory();
    static QDir getClientDirectory();
    static QDir getLogsDirectory();

    static QString getContentCachePath();
    static QString getClientExecutablePath();
    static QString getConfigFilePath();
    static QString getLauncherFilePath();
};
