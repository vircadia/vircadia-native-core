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
import "./wallet" as HifiWallet

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: checkoutRoot;
    property bool inventoryReceived: false;
    property bool balanceReceived: false;
    property string itemId: "";
    property string itemHref: "";
    property int balanceAfterPurchase: 0;
    property bool alreadyOwned: false;
    property int itemPriceFull: 0;
    // Style
    color: hifi.colors.baseGray;
    Hifi.QmlCommerce {
        id: commerce;
        onBuyResult: {
            if (result.status !== 'success') {
                buyButton.text = result.message;
                buyButton.enabled = false;
            } else {
                if (urlHandler.canHandleUrl(itemHref)) {
                    urlHandler.handleUrl(itemHref);
                }
                sendToScript({method: 'checkout_buySuccess', itemId: itemId});
            }
        }
        onBalanceResult: {
            if (result.status !== 'success') {
                console.log("Failed to get balance", result.message);
            } else {
                balanceReceived = true;
                hfcBalanceText.text = parseFloat(result.data.balance/100).toFixed(2);
                balanceAfterPurchase = parseFloat(result.data.balance/100) - parseFloat(checkoutRoot.itemPriceFull/100).toFixed(2);
            }
        }
        onInventoryResult: {
            if (result.status !== 'success') {
                console.log("Failed to get inventory", result.message);
            } else {
                inventoryReceived = true;
                console.log('inventory fixme', JSON.stringify(result));
                if (inventoryContains(result.data.assets, itemId)) {
                    alreadyOwned = true;
                } else {
                    alreadyOwned = false;
                }
            }
        }
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
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.bottom: parent.bottom;
            width: paintedWidth;
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

        // HFC Balance text
        Item {
            id: hfcBalanceContainer;
            // Anchors
            anchors.top: itemAuthorContainer.bottom;
            anchors.topMargin: 16;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: childrenRect.height;

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
                text: "--";
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

        // Item Price text
        Item {
            id: itemPriceContainer;
            // Anchors
            anchors.top: hfcBalanceContainer.bottom;
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

        // HFC "Balance After Purchase" text
        Item {
            id: hfcBalanceAfterPurchaseContainer;
            // Anchors
            anchors.top: itemPriceContainer.bottom;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: childrenRect.height;

            RalewaySemiBold {
                id: hfcBalanceAfterPurchaseTextLabel;
                text: "HFC Balance After Purchase:";
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
                id: hfcBalanceAfterPurchaseText;
                text: balanceAfterPurchase;
                // Text size
                size: hfcBalanceAfterPurchaseTextLabel.size;
                // Anchors
                anchors.top: parent.top;
                anchors.left: hfcBalanceAfterPurchaseTextLabel.right;
                anchors.leftMargin: 16;
                width: paintedWidth;
                // Style
                color: (balanceAfterPurchase >= 0) ? hifi.colors.lightGrayText : hifi.colors.redHighlight;
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
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;

        // "Cancel" button
        HifiControlsUit.Button {
            id: cancelButton;
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
            id: buyButton;
            enabled: balanceAfterPurchase >= 0 && inventoryReceived && balanceReceived;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.topMargin: 3;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 3;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            width: parent.width/2 - anchors.rightMargin*2;
            text: (inventoryReceived && balanceReceived) ? (alreadyOwned ? "Already Owned: Get Item" : "Buy") : "--";
            onClicked: {
                if (!alreadyOwned) {
                    commerce.buy(itemId, parseFloat(itemPriceText.text*100));
                } else {
                    if (urlHandler.canHandleUrl(itemHref)) {
                        urlHandler.handleUrl(itemHref);
                    }
                    sendToScript({method: 'checkout_buySuccess', itemId: itemId});
                }
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
                checkoutRoot.itemPriceFull = message.params.itemPrice;
                itemPriceText.text = parseFloat(checkoutRoot.itemPriceFull/100).toFixed(2);
                itemHref = message.params.itemHref;
                commerce.balance();
                commerce.inventory();
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    function inventoryContains(inventoryJson, id) {
        for (var idx = 0; idx < inventoryJson.length; idx++) {
            if(inventoryJson[idx].id === id) {
                return true;
            }
        }
        return false;
    }

    //
    // FUNCTION DEFINITIONS END
    //
}
