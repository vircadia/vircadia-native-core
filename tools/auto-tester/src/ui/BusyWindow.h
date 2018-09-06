//
//  BusyWindow.h
//
//  Created by Nissim Hadar on 29 Jul 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_BusyWindow_h
#define hifi_BusyWindow_h

#include "ui_BusyWindow.h"

class BusyWindow : public QDialog, public Ui::BusyWindow {
    Q_OBJECT

public:
    BusyWindow(QWidget* parent = Q_NULLPTR);
};

#endif