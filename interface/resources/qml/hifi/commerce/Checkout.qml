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
    property string itemId; 
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
        height: childrenRect.height + 20;
        // Anchors
        anchors.left: parent.left;
        anchors.top: titleBarContainer.bottom;

        // Item Name text
        Item {
            id: itemNameContainer; 
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: childrenRect.height;

            RalewaySemiBold {
                id: itemNameTextLabel;
                text: "Item Name:";
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                width: paintedWidth;
                // Text size
                size: 16;
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
                anchors.top: parent.top;
                anchors.left: itemNameTextLabel.right;
                anchors.leftMargin: 16;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                horizontalAlignment: Text.AlignHLeft;
                verticalAlignment: Text.AlignVCenter;
            }
        }
    
        
        // Item Author text
        Item {
            id: itemAuthorContainer; 
            // Anchors
            anchors.top: itemNameContainer.bottom;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: childrenRect.height;

            RalewaySemiBold {
                id: itemAuthorTextLabel;
                text: "Item Author:";
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                width: paintedWidth;
                // Text size
                size: 16;
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
                anchors.top: parent.top;
                anchors.left: itemAuthorTextLabel.right;
                anchors.leftMargin: 16;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                horizontalAlignment: Text.AlignHLeft;
                verticalAlignment: Text.AlignVCenter;
            }
        }
        
        // Item Price text
        Item {
            id: itemPriceContainer; 
            // Anchors
            anchors.top: itemAuthorContainer.bottom;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: childrenRect.height;

            RalewaySemiBold {
                id: itemPriceTextLabel;
                text: "Item Price:";
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
                id: itemPriceText;
                // Text size
                size: itemPriceTextLabel.size;
                // Anchors
                anchors.top: parent.top;
                anchors.left: itemPriceTextLabel.right;
                anchors.leftMargin: 16;
                width: paintedWidth;
                // Style
                color: hifi.colors.lightGrayText;
                // Alignment
                horizontalAlignment: Text.AlignHLeft;
                verticalAlignment: Text.AlignVCenter;
            }
        }
    }
    //
    // ITEM DESCRIPTION END
    //

    
    //
    // ACTION BUTTONS START
    //
    Item {
        id: actionButtonsContainer;
        // Size
        width: checkoutRoot.width;
        height: 40;
        // Anchors
        anchors.left: parent.left;
        anchors.top: itemDescriptionContainer.bottom;

        // "Cancel" button
        HifiControlsUit.Button {
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: parent.width/2 - anchors.leftMargin*2;
            text: "Cancel"
            onClicked: {
                sendToScript({method: 'checkout_cancelClicked', params: itemId});
            }
        }

        // "Buy" button
        HifiControlsUit.Button {
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            width: parent.width/2 - anchors.rightMargin*2;
            text: "Buy"
            onClicked: {
                sendToScript({method: 'checkout_buyClicked', params: {success: commerce.buy(itemId, parseInt(itemPriceText.text))}});
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
            case 'updateCheckoutQML':
                itemId = message.params.itemId;
                itemNameText.text = message.params.itemName;
                itemAuthorText.text = message.params.itemAuthor;
                itemPriceText.text = message.params.itemPrice;
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
