//
//  Created by Alexander Ivash on 2018/01/08
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <DependencyManager.h>
#include <QImage>
#include <QtCore/QSharedPointer>

class DesktopPreviewProvider : public QObject, public Dependency {
    SINGLETON_DEPENDENCY
    Q_OBJECT

    DesktopPreviewProvider();
    DesktopPreviewProvider(const DesktopPreviewProvider& other) = delete;

    constexpr static const char* imagePaths[] = {
        "images/preview-disabled.png", // USER
        "images/preview-privacy.png",  // SECURE_SCREEN
        "images/preview.png",          // VSYNC
    };

public:
    enum PreviewDisabledReasons {
        USER = 0,
        SECURE_SCREEN,
        VSYNC // Not settable via this interface, as VSYNC is controlled by HMD plugin..
    };
    Q_ENUM(PreviewDisabledReasons)

    static QSharedPointer<DesktopPreviewProvider> getInstance();

    QImage getPreviewDisabledImage(bool vsyncEnabled) const;
    void setPreviewDisabledReason(PreviewDisabledReasons reason);

public slots:
    void setPreviewDisabledReason(const QString& reason);

private:
    QImage& loadPreviewImage(QImage& image, const QString& path) const;

    PreviewDisabledReasons m_previewDisabledReason = { USER };

    mutable QImage m_previewDisabled[3];
};
