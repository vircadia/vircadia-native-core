//
//  HelpWindow.h
//
//  Created by Nissim Hadar on 8 Aug 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_HelpWindow_h
#define hifi_HelpWindow_h

#include "ui_HelpWindow.h"

class HelpWindow : public QDialog, public Ui::HelpWindow {
    Q_OBJECT

public:
    HelpWindow(QWidget* parent = Q_NULLPTR);
};

#endif