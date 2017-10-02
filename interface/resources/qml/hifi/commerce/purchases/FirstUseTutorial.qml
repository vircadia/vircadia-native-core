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
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "step_1";
    // Style
    color: hifi.colors.baseGray;

    //
    // "STEP 1" START
    //
    Item {
        id: step_1;
        visible: root.activeView === "step_1";
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: tutorialActionButtonsContainer.top;

        RalewayRegular {
            id: step1text;
            text: "<b>This is the first-time Purchases tutorial.</b><br><br>Here is some <b>bold text</b> " +
            "inside Step 1.";
            // Text size
            size: 24;
            // Anchors
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
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
        visible: root.activeView === "step_2";
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: tutorialActionButtonsContainer.top;

        RalewayRegular {
            id: step2text;
            text: "<b>STEP TWOOO!!!</b>";
            // Text size
            size: 24;
            // Anchors
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
    }
    //
    // "STEP 2" END
    //

    Item {
        id: tutorialActionButtonsContainer;
        // Size
        width: root.width;
        height: 70;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 24;

        // "Skip" or "Back" button
        HifiControlsUit.Button {
            id: skipOrBackButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: parent.width/2 - anchors.leftMargin*2;
            text: root.activeView === "step_1" ? "Skip" : "Back";
            onClicked: {
                if (root.activeView === "step_1") {
                    sendSignalToParent({method: 'tutorial_skipClicked'});
                } else {
                    root.activeView = "step_" + (parseInt(root.activeView.split("_")[1]) - 1);
                }
            }
        }

        // "Next" or "Finish" button
        HifiControlsUit.Button {
            id: nextButton;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            width: parent.width/2 - anchors.rightMargin*2;
            text: root.activeView === "step_2" ? "Finish" : "Next";
            onClicked: {
                // If this is the final step...
                if (root.activeView === "step_2") {
                    sendSignalToParent({method: 'tutorial_finished'});
                } else {
                    root.activeView = "step_" + (parseInt(root.activeView.split("_")[1]) + 1);
                }
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    //
    // Function Name: fromScript()
    //
    // Relevant Variables:
    // None
    //
    // Arguments:
    // message: The message sent from the JavaScript, in this case the Marketplaces JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from a script.
    //
    function fromScript(message) {
        switch (message.method) {
            case 'updatePurchases':
                referrerURL = message.referrerURL;
            break;
            case 'purchases_getIsFirstUseResult':
                if (message.isFirstUseOfPurchases && root.activeView !== "firstUseTutorial") {
                    root.activeView = "firstUseTutorial";
                } else if (!message.isFirstUseOfPurchases && root.activeView === "initialize") {
                    root.activeView = "purchasesMain";
                    commerce.inventory();
                }
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendSignalToParent(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
