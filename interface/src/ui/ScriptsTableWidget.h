//
//  ScriptsTableWidget.h
//  interface
//
//  Created by Mohammed Nafees on 04/03/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.

#ifndef __hifi__ScriptsTableWidget__
#define __hifi__ScriptsTableWidget__

#include <QDebug>
#include <QTableWidget>

class ScriptsTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit ScriptsTableWidget(QWidget *parent);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
};

#endif /* defined(__hifi__ScriptsTableWidget__) */
