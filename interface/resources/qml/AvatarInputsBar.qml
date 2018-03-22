//
//  Created by Bradley Austin Davis on 2015/06/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.4

import "./hifi/audio" as HifiAudio

Item {
    id: root;
    objectName: "AvatarInputsBar"
    property int modality: Qt.NonModal
    width: audio.width;
    height: audio.height;
    x: 10; y: 5;

    readonly property bool shouldReposition: true;

    HifiAudio.MicBar {
        id: audio;
        visible: AvatarInputs.showAudioTools;
        standalone: true;
	    dragTarget: parent;
    }
}
