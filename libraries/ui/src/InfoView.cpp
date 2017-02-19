//
//
//  InfoView.h
//
//  Created by Bradley Austin Davis 2015/04/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InfoView.h"

#include <SettingHandle.h>
#include <PathUtils.h>
#include <QXmlQuery>
#include <QDir>
const QUrl InfoView::QML{ "InfoView.qml" };
const QString InfoView::NAME{ "InfoView" };

Setting::Handle<QString> infoVersion("info-version", QString());

static bool registered{ false };

InfoView::InfoView(QQuickItem* parent) : QQuickItem(parent) {
    registerType();
}

void InfoView::registerType() {
    if (!registered) {
        qmlRegisterType<InfoView>("Hifi", 1, 0, NAME.toLocal8Bit().constData());
        registered = true;
    }
}

QString fetchVersion(const QUrl& url) {
    QXmlQuery query;
    query.bindVariable("file", QVariant(url));
    query.setQuery("string((doc($file)//input[@id='version'])[1]/@value)");
    QString r;
    query.evaluateTo(&r);
    return r.trimmed();
}

void InfoView::show(const QString& path, bool firstOrChangedOnly, QString urlQuery) {
    registerType();
    QUrl url;
    if (QDir(path).isRelative()) {
        url = QUrl::fromLocalFile(PathUtils::resourcesPath() + path);
    } else {
        url = QUrl::fromLocalFile(path);
    }
    url.setQuery(urlQuery);

    if (firstOrChangedOnly) {
        const QString lastVersion = infoVersion.get();
        const QString version = fetchVersion(url);
        // If we have version information stored
        if (lastVersion != QString::null) {
            // Check to see the document version.  If it's valid and matches
            // the stored version, we're done, so exit
            if (version == QString::null || version == lastVersion) {
                return;
            }
        }
        infoVersion.set(version);
    }
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    QString infoViewName(NAME + "_" + path);
    offscreenUi->show(QML, NAME + "_" + path, [=](QQmlContext* context, QObject* newObject){
        QQuickItem* item = dynamic_cast<QQuickItem*>(newObject);
        item->setWidth(1024);
        item->setHeight(720);
        InfoView* newInfoView = newObject->findChild<InfoView*>();
        Q_ASSERT(newInfoView);
        newInfoView->parent()->setObjectName(infoViewName);
        newInfoView->setUrl(url);
    });
}

QUrl InfoView::url() {
    return _url;
}

void InfoView::setUrl(const QUrl& url) {
    if (url != _url) {
        _url = url;
        emit urlChanged();
    }
}
