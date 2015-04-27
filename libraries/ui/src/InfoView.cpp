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

InfoView::InfoView(QQuickItem* parent) : QQuickItem(parent) {

}

void InfoView::registerType() { 
    qmlRegisterType<InfoView>("Hifi", 1, 0, NAME.toLocal8Bit().constData()); 
} 

QString fetchVersion(const QUrl& url) {
    QXmlQuery query;
    query.bindVariable("file", QVariant(url)); 
    query.setQuery("string((doc($file)//input[@id='version'])[1]/@value)");
    QString r;
    query.evaluateTo(&r);
    return r.trimmed();
}

void InfoView::show(const QString& path, bool firstOrChangedOnly) {
    static bool registered{ false };
    if (!registered) {
        registerType();
        registered = true;
    }
    QUrl url;
    if (QDir(path).isRelative()) {
        url = QUrl::fromLocalFile(PathUtils::resourcesPath() + path); 
    } else {
        url = QUrl::fromLocalFile(path);
    }
    if (firstOrChangedOnly) {
        const QString lastVersion = infoVersion.get();
        // If we have version information stored
        if (lastVersion != QString::null) {
            // Check to see the document version.  If it's valid and matches 
            // the stored version, we're done, so exit
            const QString version = fetchVersion(url);
            if (version == QString::null || version == lastVersion) {
                return;
            }
        }
    }
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    QString infoViewName(NAME + "_" + path);
    offscreenUi->show(QML, NAME + "_" + path, [=](QQmlContext* context, QObject* newObject){
        QQuickItem* item = dynamic_cast<QQuickItem*>(newObject);
        item->setWidth(720);
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


//#include <QApplication>
//#include <QDesktopWidget>
//#include <QDesktopServices>
//#include <QFileInfo>
//#include <QtWebKitWidgets/QWebFrame>
//#include <QtWebKit/QWebElement>
//
//#include <PathUtils.h>
//#include <SettingHandle.h>
//
//#include "InfoView.h"
//
//static const float MAX_DIALOG_HEIGHT_RATIO = 0.9f;
//
//
//
//InfoView::InfoView(bool forced, QString path) : _forced(forced)
//{
//    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
//        
//    QString absPath = QFileInfo(PathUtils::resourcesPath() + path).absoluteFilePath();
//    QUrl url = QUrl::fromLocalFile(absPath);
//    
//    page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
//    connect(this, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClickedInfoView(QUrl)));
//    
//    load(url);
//    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loaded(bool)));
//}
//
//void InfoView::showFirstTime(QString path) {
//    new InfoView(false, path);
//}
//
//void InfoView::forcedShow(QString path) {
//    new InfoView(true, path);
//}
//
//
//void InfoView::loaded(bool ok) {
//    if (!ok || !shouldShow()) {
//        deleteLater();
//        return;
//    }
//    
//    QDesktopWidget* desktop = qApp->desktop();
//    QWebFrame* mainFrame = page()->mainFrame();
//    
//    int height = mainFrame->contentsSize().height() > desktop->height() ?
//        desktop->height() * MAX_DIALOG_HEIGHT_RATIO :
//        mainFrame->contentsSize().height();
//    
//    resize(mainFrame->contentsSize().width(), height);
//    move(desktop->screen()->rect().center() - rect().center());
//    setWindowTitle(title());
//    setAttribute(Qt::WA_DeleteOnClose);
//    show();
//}
//
//void InfoView::linkClickedInfoView(QUrl url) {
//    close();
//    QDesktopServices::openUrl(url);
//}
//#endif