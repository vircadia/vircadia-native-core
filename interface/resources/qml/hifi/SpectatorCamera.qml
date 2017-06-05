//
//  SpectatorCamera.qml
//  qml/hifi
//
//  Spectator Camera
//
//  Created by Zach Fox on 2017-06-05
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import "../styles-uit"
import "../controls-uit" as HifiControlsUit
import "../controls" as HifiControls

// references HMD, XXX from root context

Rectangle {
    id: spectatorCamera;
    // Size
    width: parent.width;
    height: parent.height;
    // Style
    color: "#E3E3E3";
    // Properties

    HifiConstants { id: hifi; }

    function fromScript(message) {
        switch (message.method) {
        case 'XXX':
        break;
        default:
            console.log('Unrecognized message from spectatorCamera.js:', JSON.stringify(message));
        }
    }
}
