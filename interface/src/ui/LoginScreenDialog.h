//
//  LoginScreenDialog.h
//  interface/src/ui
//
//  Created by Wayne Chen on 2018/10/9
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
class HMDLoginScreenDialog : public QmlWindowClass {
    virtual QString qmlSource() const override { return "hifi/dialogs/.qml"; }
};
