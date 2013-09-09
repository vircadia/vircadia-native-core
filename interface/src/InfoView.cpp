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

#define VIEW_FIXED_WIDTH 808
#define SETTINGS_KEY_VERSION "info-version"

InfoView::InfoView()
{
    this->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);

#ifdef Q_OS_MAC
    QString resourcesPath = QCoreApplication::applicationDirPath() + "/../Resources";
#else
    QString resourcesPath = QCoreApplication::applicationDirPath() + "/resources";
#endif

    QUrl url = QUrl::fromLocalFile(resourcesPath + "/html/interface-welcome-allsvg.html");
    this->load(url);
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loaded(bool)));
}

void InfoView::showFirstTime()
{
    new InfoView();
}

void InfoView::loaded(bool ok)
{
    QSettings* settings = Application::getInstance()->getSettings();
    
    QString lastVersion = settings->value(SETTINGS_KEY_VERSION).toString();
    
    
    QWebFrame* mainFrame = this->page()->mainFrame();
    QWebElement versionTag = mainFrame->findFirstElement("#version");
    QString version = versionTag.attribute("value");
    
    if (lastVersion == QString::null || version == QString::null || lastVersion != version) {
        if (version != QString::null) {
            settings->setValue(SETTINGS_KEY_VERSION, version);
        }
        
        QDesktopWidget* desktop = Application::getInstance()->desktop();
        
        int h = desktop->height();
        this->resize(VIEW_FIXED_WIDTH, h * 0.8);
        
        int w = desktop->width();
        int mw = this->size().width();
        int mh = this->size().height();
        int cw = (w/2) - (mw/2);
        int ch = (h/2) - (mh/2);
        this->move(cw, ch);

        this->setWindowTitle(this->title());
        this->show();
    }
}
