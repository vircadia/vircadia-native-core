//
//  ScriptsTableWidget.h
//  interface
//
//  Created by Mohammed Nafees on 04/03/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__ScriptsTableWidget_h
#define hifi__ScriptsTableWidget_h

#include <QDebug>
#include <QTableWidget>

class ScriptsTableWidget : public QTableWidget {
    Q_OBJECT
public:
    explicit ScriptsTableWidget(QWidget* parent);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
};

#endif // hifi__ScriptsTableWidget_h
