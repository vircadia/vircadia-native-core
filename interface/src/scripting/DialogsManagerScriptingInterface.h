//
//  DialogsManagerScriptingInterface.h
//  interface/src/scripting
//
//  Created by Zander Otavka on 7/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DialogsManagerScriptInterface_h
#define hifi_DialogsManagerScriptInterface_h

#include <QObject>

class DialogsManagerScriptingInterface : public QObject {
    Q_OBJECT
public:
    DialogsManagerScriptingInterface();
    Q_INVOKABLE void showFeed();

public slots:
    void toggleAddressBar();

signals:
    void addressBarToggled();
    void addressBarShown(bool visible);
};

#endif
