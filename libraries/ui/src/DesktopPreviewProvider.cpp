#include "DesktopPreviewProvider.h"
#include <PathUtils.h>
#include <QMetaEnum>
#include <QtPlugin>
#include <cassert>

DesktopPreviewProvider::DesktopPreviewProvider() {
}

constexpr const char* DesktopPreviewProvider::imagePaths[];

QSharedPointer<DesktopPreviewProvider> DesktopPreviewProvider::getInstance() {
    static QSharedPointer<DesktopPreviewProvider> instance = DependencyManager::get<DesktopPreviewProvider>();
    return instance;
}

QImage DesktopPreviewProvider::getPreviewDisabledImage(bool vsyncEnabled) const {

    auto imageIndex = vsyncEnabled ? VSYNC : m_previewDisabledReason;
    assert(imageIndex >= 0 && imageIndex <= VSYNC);

    return !m_previewDisabled[imageIndex].isNull() ? m_previewDisabled[imageIndex] : loadPreviewImage(m_previewDisabled[imageIndex], PathUtils::resourcesPath() + imagePaths[imageIndex]);
}

void DesktopPreviewProvider::setPreviewDisabledReason(PreviewDisabledReasons reason) {
    if (reason == VSYNC) {
        qDebug() << "Preview disabled reason can't be forced to " << QMetaEnum::fromType<DesktopPreviewProvider::PreviewDisabledReasons>().valueToKey(reason);
        return; // Not settable via this interface, as VSYNC is controlled by HMD plugin..
    }

    m_previewDisabledReason = reason;
}

void DesktopPreviewProvider::setPreviewDisabledReason(const QString& reasonString) {
    PreviewDisabledReasons reason = USER;
    bool ok = false;

    reason = (PreviewDisabledReasons) QMetaEnum::fromType<DesktopPreviewProvider::PreviewDisabledReasons>().keyToValue(reasonString.toLatin1().data(), &ok);
    if (ok) {
        setPreviewDisabledReason(reason);
    }
}

QImage& DesktopPreviewProvider::loadPreviewImage(QImage& image, const QString& path) const {
    return image = QImage(path).mirrored().convertToFormat(QImage::Format_RGBA8888);
}
