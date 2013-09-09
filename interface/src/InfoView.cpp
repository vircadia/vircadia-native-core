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

InfoView::InfoView()
{
    settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);

#ifdef Q_OS_MAC
    QString resourcesPath = QCoreApplication::applicationDirPath() + "/../Resources";
#else
    QString resourcesPath = QCoreApplication::applicationDirPath() + "/resources";
#endif

    QUrl url = QUrl::fromLocalFile(resourcesPath + "/html/interface-welcome-allsvg.html");
    load(url);
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loaded(bool)));
}

void InfoView::showFirstTime()
{
    new InfoView();
}

void InfoView::loaded(bool ok)
{
    QSettings* settings = Application::getInstance()->getSettings();
    
    QString lastVersion = settings->value(SETTINGS_VERSION_KEY).toString();
    
    QWebFrame* mainFrame = page()->mainFrame();
    QWebElement versionTag = mainFrame->findFirstElement("#version");
    QString version = versionTag.attribute("value");
    
    if (lastVersion == QString::null || version == QString::null || lastVersion != version) {
        if (version != QString::null) {
            settings->setValue(SETTINGS_VERSION_KEY, version);
        }
        
        QDesktopWidget* desktop = Application::getInstance()->desktop();
        int height = mainFrame->contentsSize().height() > desktop->height() ?
            desktop->height() * 0.9 :
            mainFrame->contentsSize().height();
        
        resize(mainFrame->contentsSize().width(), height);
        move(desktop->screen()->rect().center() - rect().center());
        setWindowTitle(title());
        show();
        setWindowModality(Qt::WindowModal);
    }
}
