//
//  AboutUtil.cpp
//  interface/src
//
//  Created by Vlad Stelmahovsky on 15/5/2018.
//  Copyright 2018 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AboutUtil.h"
#include <QDate>
#include <QLocale>

#include <ui/TabletScriptingInterface.h>
#include <OffscreenQmlDialog.h>

#include "BuildInfo.h"
#include "DependencyManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "Application.h"

AboutUtil::AboutUtil(QObject *parent) : QObject(parent) {
    QLocale locale;
    _dateConverted = QDate::fromString(BuildInfo::BUILD_TIME, "dd/MM/yyyy").
            toString(locale.dateFormat(QLocale::ShortFormat));
}

AboutUtil *AboutUtil::getInstance() {
    static AboutUtil instance;
    return &instance;
}

QString AboutUtil::getBuildDate() const {
    return _dateConverted;
}

QString AboutUtil::getBuildVersion() const {
    return BuildInfo::VERSION;
}

QString AboutUtil::getReleaseName() const {
    return BuildInfo::RELEASE_NAME;
}

QString AboutUtil::getQtVersion() const {
    return qVersion();
}

void AboutUtil::openUrl(const QString& url) const {
    auto abboutUtilInstance = AboutUtil::getInstance();
    if (!abboutUtilInstance) {
        return;
    }

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(abboutUtilInstance, "openUrl", Q_ARG(const QString&, url));
        return;
    }

    auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    auto offscreenUI = DependencyManager::get<OffscreenUi>();

    if (tablet->getToolbarMode() && offscreenUI) {
        offscreenUI->load("Browser.qml", [=](QQmlContext* context, QObject* newObject) {
            newObject->setProperty("url", url);
        });
    } else {
        if (!hmd->getShouldShowTablet() && !qApp->isHMDMode() && offscreenUI) {
            offscreenUI->load("Browser.qml", [=](QQmlContext* context, QObject* newObject) {
                newObject->setProperty("url", url);
            });
        } else {
            tablet->gotoWebScreen(url);
        }
    }
}
