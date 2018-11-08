//
//  MessageDialogButton.qml
//
//  Created by Bradley Austin Davis on 29 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Dialogs 1.2

import controlsUit 1.0

Button {
    property var dialog;
    property int button: StandardButton.Ok;

    color: focus ? hifi.buttons.blue : hifi.buttons.white
    onClicked: dialog.click(button)
    visible: dialog.buttons & button
    Keys.onPressed: {
        if (!focus) {
            return
        }
        switch (event.key) {
        case Qt.Key_Enter:
        case Qt.Key_Return:
            event.accepted = true
            dialog.click(button)
            break
        }
    }
}
