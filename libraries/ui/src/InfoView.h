//
//  InfoView.h
//
//  Created by Bradley Austin Davis 2015/04/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InfoView_h
#define hifi_InfoView_h

#include <QQuickItem>
#include "OffscreenUi.h"

class InfoView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)

    static const QUrl QML;
    static const QString NAME;
public:
    static void registerType();
    static void show(const QString& path, bool firstOrChangedOnly = false, QString urlQuery = "");

    InfoView(QQuickItem* parent = nullptr);
    QUrl url();
    void setUrl(const QUrl& url);

signals:
    void urlChanged();

private:
    QUrl _url;
};

#endif // hifi_InfoView_h
