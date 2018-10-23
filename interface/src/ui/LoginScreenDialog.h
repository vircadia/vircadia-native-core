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
#ifndef hifi_LoginScreenDialog_h
#define hifi_LoginScreenDialog_h

#include <DependencyManager.h>

class PointerEvent;

class LoginScreenDialog : public Dependency, QObject {
    LoginScreenDialog();
    void createLoginScreen();

public slots:
};

#endif  // hifi_LoginScreenDialog_h