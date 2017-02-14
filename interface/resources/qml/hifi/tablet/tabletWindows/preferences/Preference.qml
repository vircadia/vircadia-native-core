//
//  Preference.qml
//
//  Created by Bradley Dante Ruiz on 13 Feb 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: root
    anchors { left: parent.left; right: parent.right }
    property var preference;
    property string label: preference ? preference.name : "";
    property bool isFirstCheckBox;
    Component.onCompleted: {
        if (preference) {
            preference.load();
            enabled = Qt.binding(function() { return preference.enabled; } );
        }
    }

    function restore() { }
}
