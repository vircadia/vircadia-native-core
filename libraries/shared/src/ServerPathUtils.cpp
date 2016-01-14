#include "ServerPathUtils.h"

#include <QStandardPaths>
#include <QtCore/QDir>

QString ServerPathUtils::getDataDirectory() {
    auto directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty()) {
        directory = '.';
    }
    return directory;
}

QString ServerPathUtils::getDataFilePath(QString filename) {
    return QDir(getDataDirectory()).absoluteFilePath(filename);
}
