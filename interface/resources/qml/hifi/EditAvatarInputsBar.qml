//
//  EditAvatarInputsBar.qml
//  qml/hifi
//
//  Audio setup
//
//  Created by Wayne Chen on 3/20/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../windows"

Rectangle {
    id: editRect

    HifiConstants { id: hifi; }

    color: hifi.colors.baseGray;

    signal sendToScript(var message);
    function emitSendToScript(message) {
        sendToScript(message);
    }

    function fromScript(message) {
    }

}
