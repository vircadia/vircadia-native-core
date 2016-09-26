//
//  Value.qml
//  scripts/developer/utilities/audio
//
//  Created by Zach Pomerantz on 9/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

RowLayout {
    id: value
    property string label
    property var source

    width: parent.width
    property int dataPixelWidth: 150

    Label {
        Layout.preferredWidth: dataPixelWidth
        text: value.label
    }
    Label {
        Layout.preferredWidth: 0
        horizontalAlignment: Text.AlignRight
        text: value.source
    }
}

