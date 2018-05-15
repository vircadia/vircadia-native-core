//
//  AboutUtil.cpp
//  interface/src
//
//  Created by Vlad Stelmahovsky on 15/5/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AboutUtil.h"
#include "BuildInfo.h"
#include <ui/TabletScriptingInterface.h>
#include "DependencyManager.h"
#include "scripting/HMDScriptingInterface.h"
#include "Application.h"
#include <OffscreenQmlDialog.h>

AboutUtil::AboutUtil(QObject *parent) : QObject(parent) {}

AboutUtil *AboutUtil::getInstance()
{
    static AboutUtil instance;
    return &instance;
}

QString AboutUtil::buildDate() const
{
    return BuildInfo::BUILD_TIME;
}

QString AboutUtil::buildVersion() const
{
    return BuildInfo::VERSION;
}

QString AboutUtil::qtVersion() const
{
    return qVersion();
}

void AboutUtil::openUrl(const QString& url) const {

    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    if (tablet->getToolbarMode()) {
        offscreenUi->load("Browser.qml", [=](QQmlContext* context, QObject* newObject) {
            newObject->setProperty("url", url);
        });
    } else {
        if (!hmd->getShouldShowTablet() && !qApp->isHMDMode()) {
            offscreenUi->load("Browser.qml", [=](QQmlContext* context, QObject* newObject) {
                newObject->setProperty("url", url);
            });
        } else {
            tablet->gotoWebScreen(url);
        }
    }
}
