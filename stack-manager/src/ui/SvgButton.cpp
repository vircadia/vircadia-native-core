//
//  SvgButton.cpp
//  StackManagerQt/src/ui
//
//  Created by Mohammed Nafees on 10/20/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#include "SvgButton.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QCursor>

SvgButton::SvgButton(QWidget* parent) :
    QAbstractButton(parent)
{
}

void SvgButton::enterEvent(QEvent*) {
    setCursor(QCursor(Qt::PointingHandCursor));
}

void SvgButton::setSvgImage(const QString& svg) {
    _svgImage = svg;
}

void SvgButton::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QSvgRenderer renderer(_svgImage);
    renderer.render(&painter);
}
