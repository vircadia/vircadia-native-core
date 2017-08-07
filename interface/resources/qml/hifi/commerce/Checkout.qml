//
//  Checkout.qml
//  qml/hifi/commerce
//
//  Checkout
//
//  Created by Zach Fox on 2017-08-07
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

    id: checkoutRoot;
    // Style
    color: hifi.colors.baseGray;

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        width: checkoutRoot.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "Checkout";
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
    // ITEM DESCRIPTION START
    //
    Item {
        id: itemDescriptionContainer;
        // Size
        width: checkoutRoot.width;
        height: 150;
        // Anchors
        anchors.left: parent.left;
        anchors.top: titleBarContainer.bottom;

        // Item Name text
        RalewaySemiBold {
            id: itemNameTextLabel;
            text: "Item Name:";
            // Text size
            size: 16;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
        RalewayRegular {
            id: itemNameText;
            // Text size
            size: itemNameTextLabel.size;
            // Anchors
            anchors.top: itemNameTextLabel.top;
            anchors.topMargin: itemNameTextLabel.anchors.topMargin;
            anchors.left: itemNameTextLabel.right;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
        
        // Item Author text
        RalewaySemiBold {
            id: itemAuthorTextLabel;
            text: "Item Author:";
            // Text size
            size: 16;
            // Anchors
            anchors.top: itemNameTextLabel.bottom;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
        RalewayRegular {
            id: itemAuthorText;
            // Text size
            size: itemAuthorTextLabel.size;
            // Anchors
            anchors.top: itemAuthorTextLabel.top;
            anchors.topMargin: itemAuthorTextLabel.anchors.topMargin;
            anchors.left: itemAuthorTextLabel.right;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
        
        // Item Price text
        RalewaySemiBold {
            id: itemPriceTextLabel;
            text: "Item Price:";
            // Text size
            size: 16;
            // Anchors
            anchors.top: itemAuthorTextLabel.bottom;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            // Style
            color: hifi.colors.lightGrayText;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
        RalewayRegular {
            id: itemPriceText;
            // Text size
            size: itemPriceTextLabel.size;
            // Anchors
            anchors.top: itemPriceTextLabel.top;
            anchors.topMargin: itemPriceTextLabel.anchors.topMargin;
            anchors.left: itemPriceTextLabel.right;
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
    // ITEM DESCRIPTION END
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
        console.log("ZRF:", JSON.stringify(message));
        switch (message.method) {
            case 'updateCheckoutQML':
                console.log("ZRF:", JSON.stringify(message));
                itemNameText.text = message.itemName;
                itemAuthorText.text = message.itemAuthor;
                itemAuthorText.text = message.itemPrice;
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
