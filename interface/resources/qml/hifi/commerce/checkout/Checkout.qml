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
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls
import "../wallet" as HifiWallet

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "checkoutMain";
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

        onSecurityImageResult: {
            if (!exists) { // "If security image is not set up"
                if (root.activeView !== "notSetUp") {
                    notSetUp.visible = true;
                }
            }
        }

        onKeyFilePathResult: {
            if (path === "" && root.activeView !== "notSetUp") {
                notSetUp.visible = true;
            }
        }

        onBuyResult: {
            if (result.status !== 'success') {
                console.log("ZRF " + JSON.stringify(result));
                failureErrorText.text = "Here's some more info about the error:<br><br>" + (result.message);
                root.activeView = "checkoutFailure";
            } else {
                root.activeView = "checkoutSuccess";
            }
        }
        onBalanceResult: {
            if (result.status !== 'success') {
                console.log("Failed to get balance", result.data.message);
            } else {
                balanceReceived = true;
                hfcBalanceText.text = parseFloat(result.data.balance/100).toFixed(2);
                balanceAfterPurchase = parseFloat(result.data.balance/100) - parseFloat(root.itemPriceFull/100).toFixed(2);
            }
        }
        onInventoryResult: {
            if (result.status !== 'success') {
                console.log("Failed to get inventory", result.data.message);
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
        width: parent.width;
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        // Title Bar text
        RalewaySemiBold {
            id: titleBarText;
            text: "MARKETPLACE";
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
    
    //
    // "WALLET NOT SET UP" START
    //
    Item {
        id: notSetUp;
        visible: root.activeView === "notSetUp";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: root.bottom;
        
        RalewayRegular {
            id: notSetUpText;
            text: "Set up your Wallet (no credit card necessary) to claim your free <b>100 HFC</b> " +
            "and get items from the Marketplace.";
            // Text size
            size: 18;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 50;
            anchors.bottom: notSetUpActionButtonsContainer.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }

        Item {
            id: notSetUpActionButtonsContainer;
            // Size
            width: root.width;
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

            // "Set Up" button
            HifiControlsUit.Button {
                id: setUpButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: parent.width/2 - anchors.rightMargin*2;
                text: "Set Up Wallet"
                onClicked: {
                    sendToScript({method: 'checkout_setupClicked'});
                }
            }
        }
    }
    //
    // "WALLET NOT SET UP" END
    //

    //
    // CHECKOUT CONTENTS START
    //
    Item {
        id: checkoutContents;
        visible: root.activeView === "checkoutMain";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        //
        // ITEM DESCRIPTION START
        //
        Item {
            id: itemDescriptionContainer;
            // Size
            width: parent.width;
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
        // ACTION BUTTONS AND TEXT START
        //
        Item {
            id: checkoutActionButtonsContainer;
            // Size
            width: root.width;
            height: 130;
            // Anchors
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 8;

            // "Cancel" button
            HifiControlsUit.Button {
                id: cancelPurchaseButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                height: 40;
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
                height: 40;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: parent.width/2 - anchors.rightMargin*2;
                text: (inventoryReceived && balanceReceived) ? ("Buy") : "--";
                onClicked: {
                    buyButton.enabled = false;
                    commerce.buy(itemId, itemPriceFull);
                }
            }
        
            RalewayRegular {
                id: buyText;
                text: (balanceAfterPurchase >= 0) ? "This item will be added to your <b>Inventory</b>, which can be accessed from <b>Marketplace</b>." : "OUT OF MONEY LOL";
                // Text size
                size: 20;
                // Anchors
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 10;
                height: paintedHeight;
                anchors.left: parent.left;
                anchors.leftMargin: 10;
                anchors.right: parent.right;
                anchors.rightMargin: 10;
                // Style
                color: hifi.colors.faintGray;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
        }
        //
        // ACTION BUTTONS END
        //
    }
    //
    // CHECKOUT CONTENTS END
    //

    //
    // CHECKOUT SUCCESS START
    //
    Item {
        id: checkoutSuccess;
        visible: root.activeView === "checkoutSuccess";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: root.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        
        RalewayRegular {
            id: completeText;
            text: "<b>Purchase Complete.</b><br>You bought " + (itemNameText.text) + " by " (itemAuthorText.text);
            // Text size
            size: 24;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 10;
            height: paintedHeight;
            anchors.left: parent.left;
            anchors.right: parent.right;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
        
        Item {
            id: checkoutSuccessActionButtonsContainer;
            // Size
            width: root.width;
            height: 130;
            // Anchors
            anchors.top: completeText.bottom;
            anchors.topMargin: 10;
            anchors.left: parent.left;
            anchors.right: parent.right;

            // "Rez Now!" button
            HifiControlsUit.Button {
                id: rezNowButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: parent.width/2 - anchors.leftMargin*2;
                text: "Rez Now!"
                onClicked: {
                    if (urlHandler.canHandleUrl(itemHref)) {
                        urlHandler.handleUrl(itemHref);
                    }
                }
            }

            // "Inventory" button
            HifiControlsUit.Button {
                id: inventoryButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: parent.width/2 - anchors.rightMargin*2;
                text: "View Inventory";
                onClicked: {
                    sendToScript({method: 'checkout_goToInventory'});
                }
            }
        }
        
        Item {
            id: continueShoppingButtonContainer;
            // Size
            width: root.width;
            height: 130;
            // Anchors
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 8;
            // "Continue Shopping" button
            HifiControlsUit.Button {
                id: continueShoppingButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: parent.width/2 - anchors.rightMargin*2;
                text: "Continue Shopping";
                onClicked: {
                    sendToScript({method: 'checkout_continueShopping', itemId: itemId});
                }
            }
        }
    }
    //
    // CHECKOUT SUCCESS END
    //

    //
    // CHECKOUT FAILURE START
    //
    Item {
        id: checkoutFailure;
        visible: root.activeView === "checkoutFailure";
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: root.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        
        RalewayRegular {
            id: failureHeaderText;
            text: "<b>Purchase Failed.</b><br>Your Inventory and HFC balance haven't changed.";
            // Text size
            size: 24;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 80;
            height: paintedHeight;
            anchors.left: parent.left;
            anchors.right: parent.right;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
        
        RalewayRegular {
            id: failureErrorText;
            // Text size
            size: 16;
            // Anchors
            anchors.top: failureHeaderText.bottom;
            anchors.topMargin: 35;
            height: paintedHeight;
            anchors.left: parent.left;
            anchors.right: parent.right;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
        
        Item {
            id: backToMarketplaceButtonContainer;
            // Size
            width: root.width;
            height: 130;
            // Anchors
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 8;
            // "Back to Marketplace" button
            HifiControlsUit.Button {
                id: backToMarketplaceButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: parent.width/2 - anchors.rightMargin*2;
                text: "Back to Marketplace";
                onClicked: {
                    sendToScript({method: 'checkout_continueShopping', itemId: itemId});
                }
            }
        }
    }
    //
    // CHECKOUT FAILURE END
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
                root.itemPriceFull = message.params.itemPrice;
                itemPriceText.text = parseFloat(root.itemPriceFull/100).toFixed(2);
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
