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
    width: HMD.active ? audio.width : audioApplication.width;
    height: HMD.active ? audio.height : audioApplication.height;
    x: 10;
    y: 5;
    readonly property bool shouldReposition: true;

    HifiAudio.MicBar {
        id: audio;
        visible: AvatarInputs.showAudioTools && HMD.active;
        standalone: true;
        dragTarget: parent;
    }

    HifiAudio.MicBarApplication {
        id: audioApplication;
        visible: AvatarInputs.showAudioTools && !HMD.active;
        onVisibleChanged: {
            console.log("visible changed: " + visible);
        }
        standalone: true;
        dragTarget: parent;
    }
    BubbleIcon {
        dragTarget: parent
        visible: !HMD.active;
    }
}
