//
//  Created by Bradley Austin Davis on 2015/06/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtGraphicalEffects 1.0

import "./hifi/audio" as HifiAudio

Item {
    id: root;
    objectName: "AvatarInputsBar"
    property int modality: Qt.NonModal
    width: audio.width;
    height: audio.height;
    x: 10; y: 5;

    readonly property bool shouldReposition: true;

    HifiAudio.MicBarApplication {
        id: audio;
        visible: AvatarInputs.showAudioTools;
        standalone: true;
        dragTarget: parent;
    }
    Image {
        id: bubbleIcon
        source: "../icons/tablet-icons/bubble-i.svg";
        width: 28;
        height: 28;
        anchors {
            left: root.right
            top: root.top
            topMargin: (root.height - bubbleIcon.height) / 2
        }
    }
    ColorOverlay {
        anchors.fill: bubbleIcon
        source: bubbleIcon
        color: Users.getIgnoreRadiusEnabled() ? Qt.rgba(31, 198, 166, 0.3) : Qt.rgba(255, 255, 255, 0.3);
        onColorChanged: {
            console.log("colorChanged")
        }
    }
}
