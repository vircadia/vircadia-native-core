//
//  Web3DOverlay.qml
//
//  Created by Gabriel Calero & Cristian Duarte on Jun 22, 2018
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0

Item {

    property string url
    RadialGradient {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#262626" }
            GradientStop { position: 1.0; color: "#000000" }
        }
    }

    function shortUrl(url) {
        var hostBegin = url.indexOf("://");
        if (hostBegin > -1) {
            url = url.substring(hostBegin + 3);
        }

        var portBegin = url.indexOf(":");
        if (portBegin > -1) {
            url = url.substring(0, portBegin);
        }

        var pathBegin = url.indexOf("/");
        if (pathBegin > -1) {
            url = url.substring(0, pathBegin);
        }

        if (url.length > 45) {
            url = url.substring(0, 45);
        }

        return url;
    }

    Text { 
        id: urlText
        text: shortUrl(url)
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent
        anchors.rightMargin: 10
        anchors.leftMargin: 10
        font.family: "Cairo"
        font.weight: Font.DemiBold
        font.pointSize: 48
        fontSizeMode: Text.Fit
        color: "#FFFFFF"
        minimumPixelSize: 5
    }

    Image {
        id: hand
        source: "../../icons/hand.svg"
        width: 300
        height: 300
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 100
        anchors.rightMargin: 100
    }


}
