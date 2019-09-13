#pragma once

#include <QObject>
#include <QString>
class PathUtils : public QObject {
    Q_OBJECT
public:
    PathUtils() = default;
    ~PathUtils() = default;
    Q_INVOKABLE static QString resourcePath(const QString& source);
};
