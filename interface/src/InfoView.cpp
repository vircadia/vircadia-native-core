//
//  InfoView
//  hifi
//
//  Created by Stojce Slavkovski on 9/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "InfoView.h"
#include <QApplication>
#include "Application.h"

#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QDesktopWidget>

#define SETTINGS_VERSION_KEY "info-version"
#define MAX_DIALOG_HEIGHT_RATIO 0.9

InfoView::InfoView(bool forced, QWidget* parent) :
    QWebView(parent),
    _forced(forced) {
    
    switchToResourcesParentIfRequired();
    QString absPath = QFileInfo("resources/html/interface-welcome-allsvg.html").absoluteFilePath();
    QUrl url = QUrl::fromLocalFile(absPath);
    
    load(url);
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loaded(bool)));
}

void InfoView::showFirstTime(QWidget* parent) {
    new InfoView(false, parent);
}

void InfoView::forcedShow(QWidget* parent) {
    new InfoView(true, parent);
}

bool InfoView::shouldShow() {
    if (_forced) {
        return true;
    }
    
    QSettings* settings = Application::getInstance()->getSettings();
    
    QString lastVersion = settings->value(SETTINGS_VERSION_KEY).toString();
    
    QWebElement versionTag = page()->mainFrame()->findFirstElement("#version");
    QString version = versionTag.attribute("value");
    
    if (version != QString::null && (lastVersion == QString::null || lastVersion != version)) {
        settings->setValue(SETTINGS_VERSION_KEY, version);
        return true;
    } else {
        return false;
    }   
}

void InfoView::loaded(bool ok) {
    if (!ok || !shouldShow()) {
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
    show();
}
