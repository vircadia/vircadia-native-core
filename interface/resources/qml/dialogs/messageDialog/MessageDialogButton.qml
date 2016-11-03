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
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2

import "../../controls-uit"

Button {
    property var dialog;
    property int button: StandardButton.NoButton;

    color: dialog.defaultButton === button ? hifi.buttons.blue : hifi.buttons.white
    focus: dialog.defaultButton === button
    onClicked: dialog.click(button)
    visible: dialog.buttons & button
}
