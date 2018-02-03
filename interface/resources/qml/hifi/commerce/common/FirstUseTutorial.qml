//
//  FirstUseTutorial.qml
//  qml/hifi/commerce/purchases
//
//  FirstUseTutorial
//
//  Created by Zach Fox on 2017-09-13
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
    HifiConstants { id: hifi; }

    id: root;
    property int activeView: 1;

    onVisibleChanged: {
        if (visible) {
            root.activeView = 1;
        }
    }

    Image {
        anchors.fill: parent;
        source: "images/Purchase-First-Run-" + root.activeView + ".jpg";
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    Item {
        id: header;
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        height: childrenRect.height;

        Image {
            id: marketplaceHeaderImage;
            source: "../common/images/marketplaceHeaderImage.png";
            anchors.top: parent.top;
            anchors.topMargin: 2;
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            width: 140;
            height: 58;
            fillMode: Image.PreserveAspectFit;
            visible: false;
        }
        ColorOverlay {
            anchors.fill: marketplaceHeaderImage;
            source: marketplaceHeaderImage;
            color: "#FFFFFF"
        }
        RalewayRegular {
            id: introText1;
            text: "INTRODUCTION TO";
            // Text size
            size: 15;
            // Anchors
            anchors.top: marketplaceHeaderImage.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.leftMargin: 12;
            anchors.right: parent.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
        }
        RalewayRegular {
            id: introText2;
            text: "My Purchases";
            // Text size
            size: 28;
            // Anchors
            anchors.top: introText1.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 12;
            anchors.right: parent.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
        }
    }

    //
    // "STEP 1" START
    //
    Item {
        id: step_1;
        visible: root.activeView === 1;
        anchors.top: header.bottom;
        anchors.topMargin: 100;
        anchors.left: parent.left;
        anchors.leftMargin: 30;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        RalewayRegular {
            id: step1text;
            text: "The <b>'REZ IT'</b> button makes your purchase appear in front of you.";
            // Text size
            size: 20;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            width: 180;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
            wrapMode: Text.WordWrap;
        }
        
        // "Next" button
        HifiControlsUit.Button {
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: step1text.bottom;
            anchors.topMargin: 16;
            anchors.left: parent.left;
            width: 150;
            height: 40;
            text: "Next";
            onClicked: {
                root.activeView++;
            }
        }
        
        // "SKIP" button
        HifiControlsUit.Button {
            color: hifi.buttons.noneBorderlessGray;
            colorScheme: hifi.colorSchemes.dark;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 32;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            width: 150;
            height: 40;
            text: "SKIP";
            onClicked: {
                sendSignalToParent({method: 'tutorial_finished'});
            }
        }
    }
    //
    // "STEP 1" END
    //

    //
    // "STEP 2" START
    //
    Item {
        id: step_2;
        visible: root.activeView === 2;
        anchors.top: header.bottom;
        anchors.topMargin: 45;
        anchors.left: parent.left;
        anchors.leftMargin: 30;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        RalewayRegular {
            id: step2text;
            text: "If you rez an item twice, the first one will disappear.";
            // Text size
            size: 20;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            width: 180;
            height: paintedHeight;
            // Style
            color: hifi.colors.white;
            wrapMode: Text.WordWrap;
        }
        
        // "GOT IT" button
        HifiControlsUit.Button {
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: step2text.bottom;
            anchors.topMargin: 16;
            anchors.left: parent.left;
            width: 150;
            height: 40;
            text: "GOT IT";
            onClicked: {
                sendSignalToParent({method: 'tutorial_finished'});
            }
        }
    }
    //
    // "STEP 2" END
    //

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendSignalToParent(var message);
    //
    // FUNCTION DEFINITIONS END
    //
}
