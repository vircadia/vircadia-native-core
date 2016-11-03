//
//  OffscreenQmlDialog.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OffscreenQmlDialog.h"

OffscreenQmlDialog::OffscreenQmlDialog(QQuickItem* parent)
  : QQuickItem(parent) { }

OffscreenQmlDialog::~OffscreenQmlDialog() {
}

void OffscreenQmlDialog::hide() {
    parent()->setProperty(OFFSCREEN_VISIBILITY_PROPERTY, false);
}

QString OffscreenQmlDialog::title() const {
    return _title;
}

void OffscreenQmlDialog::setTitle(const QString& title) {
    if (title != _title) {
        _title = title;
        emit titleChanged();
    }
}

void OffscreenQmlDialog::accept() {
    hide();
    emit accepted();
}

void OffscreenQmlDialog::reject() {
    hide();
    emit rejected();
}
