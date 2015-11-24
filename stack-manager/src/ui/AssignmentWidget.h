//
//  AssignmentWidget.h
//  StackManagerQt/src/ui
//
//  Created by Mohammed Nafees on 10/18/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_AssignmentWidget_h
#define hifi_AssignmentWidget_h

#include <QWidget>
#include <QLineEdit>
#include <QUuid>

#include "SvgButton.h"

class AssignmentWidget : public QWidget
{
    Q_OBJECT
public:
    AssignmentWidget(QWidget* parent = 0);

    bool isRunning() { return _isRunning; }

public slots:
    void toggleRunningState();

private:
    int _processID;
    bool _isRunning;
    SvgButton* _runButton;
    QLineEdit* _poolIDLineEdit;
    QUuid _scriptID;
};

#endif
