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
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Rectangle {
    property string titleText;
    property string bodyImageSource;
    property string bodyText;
    property string button1text;
    property string button1method;
    property string button2text;
    property string button2method;

    readonly property string securityPicBodyText: "When you see your Security Pic, your actions and data are securely making use of your " +
        "Wallet's private keys.<br><br>You can change your Security Pic in your Wallet.";

    id: root;
    visible: false;
    anchors.fill: parent;
    color: Qt.rgba(0, 0, 0, 0.5);
    z: 999;

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
            color: hifi.colors.baseGray;
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
            color: hifi.colors.baseGray;
            size: 20;
            verticalAlignment: Text.AlignTop;
            wrapMode: Text.WordWrap;
        }

        Item {
            id: buttons;
            anchors.top: bodyText.bottom;
            anchors.topMargin: 30;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 70;

            // Button 1
            HifiControlsUit.Button {
                color: hifi.buttons.noneBorderlessGray;
                colorScheme: hifi.colorSchemes.light;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 20;
                anchors.left: parent.left;
                anchors.leftMargin: 10;
                width: root.button2text ? parent.width/2 - anchors.leftMargin*2 : parent.width - anchors.leftMargin * 2;
                text: root.button1text;
                onClicked: {
                    eval(button1method);
                }
            }

            // Button 2
            HifiControlsUit.Button {
                visible: root.button2text;
                color: hifi.buttons.noneBorderless;
                colorScheme: hifi.colorSchemes.light;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 10;
                width: parent.width/2 - anchors.rightMargin*2;
                text: root.button2text;
                onClicked: {
                    eval(button2method);
                }
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToParent(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
