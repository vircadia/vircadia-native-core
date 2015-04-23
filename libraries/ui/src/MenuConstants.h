//
//  MenuConstants.h
//
//  Created by Bradley Austin Davis on 2015/04/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_MenuContants_h
#define hifi_MenuConstants_h

#include <QQuickItem>
#include <QHash>
#include <QList>
#include "OffscreenQmlDialog.h"

class HifiMenu : public QQuickItem
{
    Q_OBJECT
    QML_DIALOG_DECL


public:
    HifiMenu(QQuickItem * parent = nullptr);
    static void setToggleAction(const QString & name, std::function<void(bool)> f);
    static void setTriggerAction(const QString & name, std::function<void()> f);
private slots:
    void onTriggeredByName(const QString & name);
    void onToggledByName(const QString & name);
private:
    static QHash<QString, std::function<void()>> triggerActions;
    static QHash<QString, std::function<void(bool)>> toggleActions;
};

#endif // hifi_MenuConstants_h




