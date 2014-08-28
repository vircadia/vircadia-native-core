//
//  ScriptsTableWidget.cpp
//  interface
//
//  Created by Mohammed Nafees on 04/03/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>

#include "ScriptsTableWidget.h"

ScriptsTableWidget::ScriptsTableWidget(QWidget* parent) :
    QTableWidget(parent) {
    verticalHeader()->setVisible(false);
    horizontalHeader()->setVisible(false);
    setShowGrid(false);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setStyleSheet("QTableWidget { border: none; background: transparent; color: #333333; } QToolTip { color: #000000; background: #f9f6e4; padding: 2px;  }");
    setToolTipDuration(200);
    setWordWrap(true);
    setGeometry(0, 0, parent->width(), parent->height());
}

void ScriptsTableWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(viewport());
    painter.setPen(QColor::fromRgb(225, 225, 225)); // #e1e1e1

    int y = 0;
    for (int i = 0; i < rowCount(); i++) {
        painter.drawLine(5, rowHeight(i) + y, width(), rowHeight(i) + y);
        y += rowHeight(i);
    }
    painter.end();

    QTableWidget::paintEvent(event);
}

void ScriptsTableWidget::keyPressEvent(QKeyEvent* event) {
    // Ignore keys so they will propagate correctly
    event->ignore();
}
