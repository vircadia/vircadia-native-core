//
//  DesktopScriptingInterface.h
//  interface/src/scripting
//
//  Created by David Rowe on 25 Aug 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DesktopScriptingInterface_h
#define hifi_DesktopScriptingInterface_h

#include <QObject>

#include <DependencyManager.h>

class DesktopScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(int width READ getWidth)  // Physical width of screen(s) including task bars and system menus
    Q_PROPERTY(int height READ getHeight)  // Physical height of screen(s) including task bars and system menus

public:
    Q_INVOKABLE void setOverlayAlpha(float alpha);
    Q_INVOKABLE void show(const QString& path, const QString&  title);

    int getWidth();
    int getHeight();
};

#endif // hifi_DesktopScriptingInterface_h
