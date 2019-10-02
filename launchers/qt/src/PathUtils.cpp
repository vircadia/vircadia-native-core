#include "PathUtils.h"

#include <QDebug>

QUrl PathUtils::resourcePath(const QString& source) {
    QString filePath = RESOURCE_PREFIX_URL + source;
#ifdef HIFI_USE_LOCAL_FILE
    return QUrl::fromLocalFile(filePath);
#else
    return QUrl(filePath);
#endif
}
