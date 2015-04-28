//
//  MessageDialog.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "MessageDialog.h"

HIFI_QML_DEF(MessageDialog)

MessageDialog::MessageDialog(QQuickItem* parent) : OffscreenQmlDialog(parent) {
    _buttons = StandardButtons(Ok | Cancel);
}

MessageDialog::~MessageDialog() {
}

QString MessageDialog::text() const {
    return _text;
}

QString MessageDialog::informativeText() const {
    return _informativeText;
}

QString MessageDialog::detailedText() const {
    return _detailedText;
}

MessageDialog::Icon MessageDialog::icon() const {
    return _icon;
}

void MessageDialog::setVisible(bool v) {
    OffscreenQmlDialog::setVisible(v);
}

void MessageDialog::setText(const QString& arg) {
    if (arg != _text) {
        _text = arg;
        emit textChanged();
    }
}

void MessageDialog::setInformativeText(const QString& arg) {
    if (arg != _informativeText) {
        _informativeText = arg;
        emit informativeTextChanged();
    }
}

void MessageDialog::setDetailedText(const QString& arg) {
    if (arg != _detailedText) {
        _detailedText = arg;
        emit detailedTextChanged();
    }
}

void MessageDialog::setIcon(MessageDialog::Icon icon) {
    if (icon != _icon) {
        _icon = icon;
        emit iconChanged();
    }
}

void MessageDialog::setStandardButtons(StandardButtons buttons) {
    if (buttons != _buttons) {
        _buttons = buttons;
        emit standardButtonsChanged();
    }
}

void MessageDialog::click(StandardButton button) {
    // FIXME try to do it more like the standard dialog
    click(StandardButton(button), ButtonRole::NoRole);
}

MessageDialog::StandardButtons MessageDialog::standardButtons() const {
    return _buttons;
}

MessageDialog::StandardButton MessageDialog::clickedButton() const {
    return _clickedButton;
}

void MessageDialog::click(StandardButton button, ButtonRole) {
    _clickedButton = button;
    if (_resultCallback) {
        _resultCallback(QMessageBox::StandardButton(_clickedButton));
    }
    hide();
}

void MessageDialog::accept() {
    // enter key is treated like OK
    if (_clickedButton == NoButton)
        _clickedButton = Ok;
    if (_resultCallback) {
        _resultCallback(QMessageBox::StandardButton(_clickedButton));
    }
    OffscreenQmlDialog::accept();
}

void MessageDialog::reject() {
    // escape key is treated like cancel
    if (_clickedButton == NoButton)
        _clickedButton = Cancel;
    if (_resultCallback) {
        _resultCallback(QMessageBox::StandardButton(_clickedButton));
    }
    OffscreenQmlDialog::reject();
}

void MessageDialog::setResultCallback(OffscreenUi::ButtonCallback callback) {
    _resultCallback = callback;
}
