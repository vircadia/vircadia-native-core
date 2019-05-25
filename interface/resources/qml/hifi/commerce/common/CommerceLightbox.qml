//
//  CommerceLightbox.qml
//  qml/hifi/commerce/common
//
//  CommerceLightbox
//
//  Created by Zach Fox on 2017-09-19
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtGraphicalEffects 1.0
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "qrc:////qml//controls" as HifiControls

// references XXX from root context

Rectangle {
    property string titleText;
    property string bodyImageSource;
    property string bodyText;
    property string button1color: hifi.buttons.noneBorderlessGray;
    property string button1text;
    property var button1method;
    property string button2color: hifi.buttons.blue;
    property string button2text;
    property var button2method;
    property string buttonLayout: "leftright";

    readonly property string securityPicBodyText: "When you see your Security Pic, your actions and data are securely making use of your " +
        "private keys.<br><br>You can change your Security Pic via Settings > Security...";

    id: root;
    visible: false;
    anchors.fill: parent;
    color: Qt.rgba(0, 0, 0, 0.5);
    z: 999;
    
    HifiConstants { id: hifi; }

    onVisibleChanged: {
        if (!visible) {
            resetLightbox();
        }
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    Rectangle {
        anchors.centerIn: parent;
        width: parent.width - 100;
        height: childrenRect.height + 30;
        color: "white";

        RalewaySemiBold {
            id: titleText;
            text: root.titleText;
            anchors.top: parent.top;
            anchors.topMargin: 30;
            anchors.left: parent.left;
            anchors.leftMargin: 30;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            height: paintedHeight;
            color: hifi.colors.black;
            size: 24;
            verticalAlignment: Text.AlignTop;
            wrapMode: Text.WordWrap;
        }

        Image {
            id: bodyImage;
            visible: root.bodyImageSource;
            source: root.bodyImageSource ? root.bodyImageSource : ""; 
            anchors.top: root.titleText ? titleText.bottom : parent.top;
            anchors.topMargin: root.titleText ? 20 : 30;
            anchors.left: parent.left;
            anchors.leftMargin: 30;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            height: 140;
            fillMode: Image.PreserveAspectFit;
            mipmap: true;
            cache: false;
        }

        RalewayRegular {
            id: bodyText;
            text: root.bodyText;
            anchors.top: root.bodyImageSource ? bodyImage.bottom : (root.titleText ? titleText.bottom : parent.top);
            anchors.topMargin: root.bodyImageSource ? 20 : (root.titleText ? 20 : 30);
            anchors.left: parent.left;
            anchors.leftMargin: 30;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            height: paintedHeight;
            color: hifi.colors.black;
            size: 20;
            verticalAlignment: Text.AlignTop;
            wrapMode: Text.WordWrap;

            onLinkActivated: {
                sendToParent({ method: 'commerceLightboxLinkClicked', linkUrl: link });
            }
        }

        Item {
            id: buttons;
            anchors.top: bodyText.bottom;
            anchors.topMargin: 30;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: root.buttonLayout === "leftright" ? 70 : 150;

            // Button 1
            HifiControlsUit.Button {
                id: button1;
                color: root.button1color;
                colorScheme: hifi.colorSchemes.light;
                anchors.top: root.buttonLayout === "leftright" ? parent.top : parent.top;
                anchors.left: parent.left;
                anchors.leftMargin: root.buttonLayout === "leftright" ? 30 : 10;
                anchors.right: root.buttonLayout === "leftright" ? undefined : parent.right;
                anchors.rightMargin: root.buttonLayout === "leftright" ? undefined : 10;
                width: root.buttonLayout === "leftright" ? (root.button2text ? parent.width/2 - anchors.leftMargin*2 : parent.width - anchors.leftMargin * 2) :
                    (undefined);
                height: 40;
                text: root.button1text;
                onClicked: {
                    button1method();
                }
            }

            // Button 2
            HifiControlsUit.Button {
                id: button2;
                visible: root.button2text;
                color: root.button2color;
                colorScheme: hifi.colorSchemes.light;
                anchors.top: root.buttonLayout === "leftright" ? parent.top : button1.bottom;
                anchors.topMargin: root.buttonLayout === "leftright" ? undefined : 20;
                anchors.left: root.buttonLayout === "leftright" ? undefined : parent.left;
                anchors.leftMargin: root.buttonLayout === "leftright" ? undefined : 10;
                anchors.right: parent.right;
                anchors.rightMargin: root.buttonLayout === "leftright" ? 30 : 10;
                width: root.buttonLayout === "leftright" ? parent.width/2 - anchors.rightMargin*2 : undefined;
                height: 40;
                text: root.button2text;
                onClicked: {
                    button2method();
                }
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToParent(var msg);

    function resetLightbox() {
        root.titleText = "";
        root.bodyImageSource = "";
        root.bodyText = "";
        root.button1color = hifi.buttons.noneBorderlessGray;
        root.button1text = "";
        root.button1method = function() {};
        root.button2color = hifi.buttons.blue;
        root.button2text = "";
        root.button2method = function() {};
        root.buttonLayout = "leftright";
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
