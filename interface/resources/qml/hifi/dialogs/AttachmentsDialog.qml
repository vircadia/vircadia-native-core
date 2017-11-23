//
//  AttachmentsDialog.qml
//
//  Created by David Rowe on 9 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../styles-uit"
import "../../windows"
import "content"

ScrollingWindow {
    id: root
    title: "Attachments"
    objectName: "AttachmentsDialog"
    width: 600
    height: 600
    resizable: true
    destroyOnHidden: true
    minSize: Qt.vector2d(400, 500)

    HifiConstants { id: hifi }

    // This is for JS/QML communication, which is unused in the AttachmentsDialog,
    // but not having this here results in spurious warnings about a
    // missing signal
    signal sendToScript(var message);

    property var settings: Settings {
        category: "AttachmentsDialog"
        property alias x: root.x
        property alias y: root.y
        property alias width: root.width
        property alias height: root.height
    }

    function closeDialog() {
        root.destroy();
    }

    AttachmentsContent { }
}
