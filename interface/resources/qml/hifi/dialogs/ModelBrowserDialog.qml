//
//  ModelBrowserDialog.qml
//
//  Created by David Rowe on 11 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "../../windows"
import "content"

ScrollingWindow {
    id: root
    objectName: "ModelBrowserDialog"
    title: "Attachment Model"
    resizable: true
    width: 600
    height: 480
    closable: false

    //HifiConstants { id: hifi }

    property var result

    signal selected(var modelUrl)
    signal canceled()

    ModelBrowserContent {
        id: modelBrowserContent
    }
}
