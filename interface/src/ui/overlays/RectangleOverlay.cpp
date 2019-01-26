//
//  Created by Bradley Austin Davis on 2016/01/27
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RectangleOverlay.h"

QString const RectangleOverlay::TYPE = "rectangle";
QUrl const RectangleOverlay::URL(QString("hifi/overlays/RectangleOverlay.qml"));

RectangleOverlay::RectangleOverlay() : QmlOverlay(URL) {}

RectangleOverlay::RectangleOverlay(const RectangleOverlay* rectangleOverlay) 
    : QmlOverlay(URL, rectangleOverlay) { }

RectangleOverlay* RectangleOverlay::createClone() const {
    return new RectangleOverlay(this);
}
