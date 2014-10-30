//
//  InfoView.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 9/7/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InfoView.h"
#include <QApplication>
#include "Application.h"

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QDesktopWidget>

#define SETTINGS_VERSION_KEY "info-version"
#define MAX_DIALOG_HEIGHT_RATIO 0.9

InfoView::InfoView(bool forced, QString path) :
    _forced(forced)
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
        
    QString absPath = QFileInfo(Application::resourcesPath() + path).absoluteFilePath();
    QUrl url = QUrl::fromLocalFile(absPath);
    
    load(url);
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loaded(bool)));
}

void InfoView::showFirstTime(QString path) {
    new InfoView(false, path);
}

void InfoView::forcedShow(QString path) {
    new InfoView(true, path);
}

bool InfoView::shouldShow() {
    bool shouldShow = false;
    if (_forced) {
        return true;
    }
    
    QSettings* settings = Application::getInstance()->lockSettings();
    
    QString lastVersion = settings->value(SETTINGS_VERSION_KEY).toString();
    
    QWebElement versionTag = page()->mainFrame()->findFirstElement("#version");
    QString version = versionTag.attribute("value");
    
    if (version != QString::null && (lastVersion == QString::null || lastVersion != version)) {
        settings->setValue(SETTINGS_VERSION_KEY, version);
        shouldShow = true;
    } else {
        shouldShow = false;
    }
    Application::getInstance()->unlockSettings();
    return shouldShow;
}

void InfoView::loaded(bool ok) {
    if (!ok || !shouldShow()) {
        deleteLater();
        return;
    }
    
    QDesktopWidget* desktop = Application::getInstance()->desktop();
    QWebFrame* mainFrame = page()->mainFrame();
    
    int height = mainFrame->contentsSize().height() > desktop->height() ?
        desktop->height() * MAX_DIALOG_HEIGHT_RATIO :
        mainFrame->contentsSize().height();
    
    resize(mainFrame->contentsSize().width(), height);
    move(desktop->screen()->rect().center() - rect().center());
    setWindowTitle(title());
    setAttribute(Qt::WA_DeleteOnClose);
    show();
}
