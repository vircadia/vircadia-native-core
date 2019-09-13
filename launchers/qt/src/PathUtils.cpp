#include "PathUtils.h"

#include <QDebug>

QString PathUtils::resourcePath(const QString& source) {
    return QString(RESOURCE_PREFIX_URL + source);
}
