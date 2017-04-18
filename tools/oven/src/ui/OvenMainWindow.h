//
//  OvenMainWindow.h
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/6/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OvenMainWindow_h
#define hifi_OvenMainWindow_h

#include <QtCore/QPointer>
#include <QtWidgets/QMainWindow>

#include "ResultsWindow.h"

const int FIXED_WINDOW_WIDTH = 640;

class OvenMainWindow : public QMainWindow {
    Q_OBJECT
public:
    OvenMainWindow(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
    ~OvenMainWindow();

    ResultsWindow* showResultsWindow(bool shouldRaise = true);
    
private:
    QPointer<ResultsWindow> _resultsWindow;
};

#endif // hifi_OvenMainWindow_h
