//
//  ErrorDialog.cpp
//
//  Created by David Rowe on 30 May 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ErrorDialog.h"

HIFI_QML_DEF(ErrorDialog)

ErrorDialog::ErrorDialog(QQuickItem* parent) : OffscreenQmlDialog(parent) {
}

ErrorDialog::~ErrorDialog() {
}

QString ErrorDialog::text() const {
    return _text;
}

void ErrorDialog::setText(const QString& arg) {
    if (arg != _text) {
        _text = arg;
        emit textChanged();
    }
}

void ErrorDialog::accept() {
    OffscreenQmlDialog::accept();
}
