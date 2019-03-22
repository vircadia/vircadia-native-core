//
//  Created by Bradley Austin Davis on 2015/06/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtGraphicalEffects 1.0

import "./hifi/audio" as HifiAudio

import TabletScriptingInterface 1.0

Item {
    id: root;
    objectName: "AvatarInputsBar"
    property int modality: Qt.NonModal
    readonly property bool ignoreRadiusEnabled: AvatarInputs.ignoreRadiusEnabled;
    x: 10;
    y: 5;
    readonly property bool shouldReposition: true;
    property bool hmdActive: HMD.active;
    width: hmdActive ? audio.width : audioApplication.width;
    height: hmdActive ? audio.height : audioApplication.height;

    onHmdActiveChanged: {
        console.log("hmd active = " + hmdActive);
    }

    Timer {
        id: hmdActiveCheckTimer;
        interval: 500;
        repeat: true;
        onTriggered: {
            root.hmdActive = HMD.active;
        }
 
    }

    HifiAudio.MicBar {
        id: audio;
        visible: AvatarInputs.showAudioTools && root.hmdActive;
        standalone: true;
        dragTarget: parent;
    }

    HifiAudio.MicBarApplication {
        id: audioApplication;
        visible: AvatarInputs.showAudioTools && !root.hmdActive;
        standalone: true;
        dragTarget: parent;
    }
    
    Component.onCompleted: {
        HMD.displayModeChanged.connect(function(isHmdMode) {
            root.hmdActive = isHmdMode;
        });
    }

    BubbleIcon {
        dragTarget: parent
        visible: !root.hmdActive;
    }
}
