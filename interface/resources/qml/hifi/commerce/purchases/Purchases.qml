//
//  Purchases.qml
//  qml/hifi/commerce/purchases
//
//  Purchases
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
            } else if (exists && root.activeView !== "purchasesMain") {
                root.activeView = "purchasesMain";
            }
        }

        onKeyFilePathIfExistsResult: {
            if (path === "" && root.activeView !== "notSetUp") {
                root.activeView = "notSetUp";
            } else if (path !== "" && root.activeView !== "purchasesMain") {
                root.activeView = "purchasesMain";
            }
        }

        onInventoryResult: {
            if (result.status !== 'success') {
                console.log("Failed to get purchases", result.message);
            } else {
                purchasesModel.append(result.data.assets);
                filteredPurchasesModel.append(result.data.assets);
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
            text: "PURCHASES";
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
    // PURCHASES CONTENTS START
    //
    Item {
        id: purchasesContentsContainer;
        visible: root.activeView === "purchasesMain";
        // Anchors
        anchors.left: parent.left;
        anchors.leftMargin: 4;
        anchors.right: parent.right;
        anchors.rightMargin: 4;
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
                property int previousLength: 0;
                anchors.fill: parent;
                placeholderText: "Filter";

                onTextChanged: {
                    if (filterBar.text.length < previousLength) {
                        filteredPurchasesModel.clear();

                        for (var i = 0; i < purchasesModel.count; i++) {
                            filteredPurchasesModel.append(purchasesModel.get(i));
                        }
                    }

                    for (var i = 0; i < filteredPurchasesModel.count; i++) {
                        if (filteredPurchasesModel.get(i).title.toLowerCase().indexOf(filterBar.text.toLowerCase()) === -1) {
                            filteredPurchasesModel.remove(i);
                            i--;
                        }
                    }
                    previousLength = filterBar.text.length;
                }
            }
        }
        //
        // FILTER BAR END
        //

        ListModel {
            id: purchasesModel;
        }
        ListModel {
            id: filteredPurchasesModel;
        }

        ListView {
            id: purchasesContentsList;
            clip: true;
            model: filteredPurchasesModel;
            // Anchors
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;
            delegate: PurchasedItem {
                itemName: title;
                itemId: id;
                itemPreviewImageUrl: preview;
                anchors.topMargin: 12;
                anchors.bottomMargin: 12;
                
                Connections {
                    onSendToPurchases: {
                        if (msg.method === 'purchases_itemInfoClicked') {
                            sendToScript({method: 'purchases_itemInfoClicked', itemId: itemId});
                        }
                    }
                }
            }
        }
    }
    //
    // PURCHASES CONTENTS END
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
                sendToScript({method: 'purchases_backClicked', referrerURL: referrerURL});
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
            case 'updatePurchases':
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
