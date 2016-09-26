//
//  MovingValuePair.qml
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
    property string label1
    property string label2
    property var source
    property var source1
    property var source2
    property string unit: "ms"

    property int labelPixelWidth: 50
    property int dataPixelWidth: 100

    Label {
        Layout.preferredWidth: labelPixelWidth - value.spacing
        text: value.label
    }

    ColumnLayout {
        RowLayout {
            Label {
                Layout.preferredWidth: dataPixelWidth 
                text: value.label1
            }
            Label {
                Layout.preferredWidth: 0
                horizontalAlignment: Text.AlignRight
                text: value.source1 + ' ' + unit
            }
        }
        RowLayout {
            Label {
                Layout.preferredWidth: dataPixelWidth 
                text: value.label2
            }
            Label {
                Layout.preferredWidth: 0
                horizontalAlignment: Text.AlignRight
                text: value.source2 + ' ' + unit
            }
        }
    }
    Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight
        Layout.alignment: Qt.AlignRight
        text: "Placeholder"
    }

}

