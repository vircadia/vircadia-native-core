#include "PathUtils.h"

#include <QDebug>

QUrl PathUtils::resourcePath(const QString& source) {
    return QUrl(RESOURCE_PREFIX_URL + source);
}
