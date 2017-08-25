//
//  Inventory.qml
//  qml/hifi/commerce/inventory
//
//  Inventory
//
//  Created by Zach Fox on 2017-08-25
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
import "../wallet" as HifiWallet

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "initialize";
    property string referrerURL: "";
    // Style
    color: hifi.colors.baseGray;
    Hifi.QmlCommerce {
        id: commerce;

        onSecurityImageResult: {
            if (!exists && root.activeView !== "notSetUp") { // "If security image is not set up"
                root.activeView = "notSetUp";
            } else if (exists && root.activeView !== "inventoryMain") {
                root.activeView = "inventoryMain";
            }
        }

        onKeyFilePathIfExistsResult: {
            if (path === "" && root.activeView !== "notSetUp") {
                root.activeView = "notSetUp";
            } else if (path !== "" && root.activeView !== "inventoryMain") {
                root.activeView = "inventoryMain";
            }
        }

        onInventoryResult: {
            if (result.status !== 'success') {
                console.log("Failed to get inventory", result.message);
            } else {
                inventoryContentsList.model = result.data.assets;
            }
        }
    }

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "INVENTORY";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.bottom: parent.bottom;
            width: paintedWidth;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        // Separator
        HifiControlsUit.Separator {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
        }
    }
    //
    // TITLE BAR END
    //

    Rectangle {
        id: initialize;
        visible: root.activeView === "initialize";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        color: hifi.colors.baseGray;

        Component.onCompleted: {
            commerce.getSecurityImage();
            commerce.getKeyFilePathIfExists();
        }
    }

    //
    // INVENTORY CONTENTS START
    //
    Item {
        id: inventoryContentsContainer;
        visible: root.activeView === "inventoryMain";
        // Anchors
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 8;
        anchors.bottom: actionButtonsContainer.top;
        anchors.bottomMargin: 8;
        
        //
        // FILTER BAR START
        //
        Item {
            id: filterBarContainer;
            // Size
            height: 40;
            // Anchors
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: parent.top;
            anchors.topMargin: 4;

            HifiControlsUit.TextField {
                id: filterBar;
                anchors.fill: parent;
                placeholderText: "Filter";
            }
        }
        //
        // FILTER BAR END
        //

        ListView {
            id: inventoryContentsList;
            clip: true;
            // Anchors
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;
            delegate: InventoryItem {
                width: parent.width;
                height: 30;
                itemName: modelData.title;
                itemId: modelData.id;
            }
        }
    }
    //
    // INVENTORY CONTENTS END
    //

    //
    // ACTION BUTTONS START
    //
    Item {
        id: actionButtonsContainer;
        // Size
        width: parent.width;
        height: 40;
        // Anchors
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;

        // "Back" button
        HifiControlsUit.Button {
            id: backButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: parent.width/2 - anchors.leftMargin*2;
            text: "Back"
            onClicked: {
                sendToScript({method: 'inventory_backClicked', referrerURL: referrerURL});
            }
        }
    }
    //
    // ACTION BUTTONS END
    //

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
            case 'updateInventory':
                referrerURL = message.referrerURL;
                commerce.balance();
                commerce.inventory();
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
