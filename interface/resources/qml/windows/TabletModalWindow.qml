//
//  ModalWindow.qml
//
//  Created by Bradley Austin Davis on 22 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import "."

Rectangle {
    id: modalWindow
    layer.enabled: true
    property var title: "Modal"
    width: tabletRoot.width
    height: tabletRoot.height
    color: "#80000000"
}
