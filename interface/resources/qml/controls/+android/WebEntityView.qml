//
//  WebEntityView.qml
//
//  Created by Gabriel Calero & Cristian Duarte on Jun 25, 2018
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5


Item {

    property string url

    Text {
        id: webContentText
        anchors.horizontalCenter : parent.horizontalCenter
        text: "Web content\nClick to view"
        font.family: "Helvetica"
        font.pointSize: 24
        color: "#0098CA"
    }

    Text { 
        id: urlText
        text: url
        anchors.horizontalCenter : parent.horizontalCenter
        anchors.top : webContentText.bottom
        font.family: "Helvetica"
        font.pointSize: 18
        color: "#0098CA"
    }
}
