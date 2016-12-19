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

Row {
    id: thisNameCard;
    // Spacing
    spacing: 5;
    // Margins
    anchors.leftMargin: 5;

    // Properties
    property string displayName: "";
    property string userName: "";
    property int displayTextHeight: 18;
    property int usernameTextHeight: 12;

    Column {
        id: avatarImage;
        // Size
        width: parent.height - 2;
        height: width;
        Rectangle {
            anchors.fill: parent;
            radius: parent.width*0.5;
            color: "#AAA5AD";
        }
    }
    Column {
        id: textContainer;
        // Size
        width: parent.width - avatarImage.width;

        RalewaySemiBold {
            id: displayNameText;
            // Properties
            text: thisNameCard.displayName;
            elide: Text.ElideRight;
            // Size
            width: parent.width;
            // Text Size
            size: thisNameCard.displayTextHeight;
            // Text Positioning
            verticalAlignment: Text.AlignVCenter;
        }
        RalewayLight {
            id: userNameText;
            // Properties
            text: thisNameCard.userName;
            elide: Text.ElideRight;
            visible: thisNameCard.displayName;
            // Size
            width: parent.width;
            // Text Size
            size: thisNameCard.usernameTextHeight;
            // Text Positioning
            verticalAlignment: Text.AlignVCenter;
        }
    }
}
