//
//  ScriptsTableWidget.cpp
//  interface
//
//  Created by Mohammed Nafees on 04/03/2014.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.

#include "ScriptsTableWidget.h"

#include <QHeaderView>
#include <QPainter>
#include <QDebug>

ScriptsTableWidget::ScriptsTableWidget(QWidget *parent) :
    QTableWidget(parent)
{
    verticalHeader()->setVisible(false);
    horizontalHeader()->setVisible(false);
    setShowGrid(false);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setStyleSheet("background: transparent;");
    setGeometry(0, 0, parent->width(), parent->height());
}

void ScriptsTableWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.setPen(QColor::fromRgb(196, 196, 196));

    int y = 0;
    for (int i = 0; i < rowCount(); ++i) {
        painter.drawLine(0, rowHeight(i) + y, width(), rowHeight(i) + y);
        y += rowHeight(i);
    }
    painter.end();

    QTableWidget::paintEvent(event);
}
