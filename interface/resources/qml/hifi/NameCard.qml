//
//  NameCard.qml
//  qml/hifi
//
//  Created by Howard Stearns on 12/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5
import "../styles-uit"


Column {
    property string displayName: "";
    property string userName: "";
    property int displayTextHeight: 18;
    property int usernameTextHeight: 12;


    RalewaySemiBold {
        // Properties
        text: parent.displayName;
        elide: Text.ElideRight;
        // Size
        width: parent.width;
        // Text Size
        size: parent.displayTextHeight;
        // Text Positioning
        verticalAlignment: Text.AlignVCenter;
    }
    RalewayLight {
        // Properties
        text: parent.userName;
        elide: Text.ElideRight;
        visible: parent.displayName;
        // Size
        size: parent.usernameTextHeight;
        width: parent.width;
        // Text Positioning
        verticalAlignment: Text.AlignVCenter;
    }
}
