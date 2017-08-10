//
//  Inventory.qml
//  qml/hifi/commerce
//
//  Inventory
//
//  Created by Zach Fox on 2017-08-10
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: inventoryRoot;
    property string referrerURL: "";
    // Style
    color: hifi.colors.baseGray;
    Hifi.QmlCommerce {
        id: commerce;
    }

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        width: inventoryRoot.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "Inventory";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.fill: parent;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
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

    //
    // HFC BALANCE START
    //
    Item {
        id: hfcBalanceContainer; 
        // Size
        width: inventoryRoot.width;
        height: childrenRect.height + 20;
        // Anchors
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 4;

        RalewaySemiBold {
            id: hfcBalanceTextLabel;
            text: "HFC Balance:";
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            width: paintedWidth;
            // Text size
            size: 20;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
        RalewayRegular {
            id: hfcBalanceText;
            // Text size
            size: hfcBalanceTextLabel.size;
            // Anchors
            anchors.top: parent.top;
            anchors.left: hfcBalanceTextLabel.right;
            anchors.leftMargin: 16;
            width: paintedWidth;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
    }
    //
    // HFC BALANCE END
    //
    
    
    //
    // ACTION BUTTONS START
    //
    Item {
        id: actionButtonsContainer;
        // Size
        width: inventoryRoot.width;
        height: 40;
        // Anchors
        anchors.left: parent.left;
        anchors.top: hfcBalanceContainer.bottom;

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
                hfcBalanceText.text = message.hfcBalance;
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
