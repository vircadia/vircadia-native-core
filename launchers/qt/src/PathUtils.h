#pragma once

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
};
