//
//  PurchasedItem.qml
//  qml/hifi/commerce/purchases
//
//  PurchasedItem
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
    property string itemName: "";
    property string itemId: "";
    property string itemPreviewImageUrl: "";
    property string itemHref: "";
    // Style
    color: hifi.colors.white;
    // Size
    width: parent.width;
    height: 120;

    Image {
        id: itemPreviewImage;
        source: root.itemPreviewImageUrl;
        anchors.left: parent.left;
        anchors.leftMargin: 8;
        anchors.top: parent.top;
        anchors.topMargin: 8;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;
        width: 180;
        fillMode: Image.PreserveAspectFit;

        MouseArea {
            anchors.fill: parent;
            onClicked: {
                sendToPurchases({method: 'purchases_itemInfoClicked', itemId: root.itemId});
            }
        }
    }

    
    RalewayRegular {
        id: itemName;
        anchors.top: itemPreviewImage.top;
        anchors.left: itemPreviewImage.right;
        anchors.leftMargin: 8;
        anchors.right: parent.right;
        anchors.rightMargin: 8;
        height: 30;
        // Text size
        size: 20;
        // Style
        color: hifi.colors.blueAccent;
        text: root.itemName;
        elide: Text.ElideRight;
        // Alignment
        horizontalAlignment: Text.AlignLeft;
        verticalAlignment: Text.AlignVCenter;

        MouseArea {
            anchors.fill: parent;
            hoverEnabled: enabled;
            onClicked: {
                sendToPurchases({method: 'purchases_itemInfoClicked', itemId: root.itemId});
            }
            onEntered: {
                itemName.color = hifi.colors.blueHighlight;
            }
            onExited: {
                itemName.color = hifi.colors.blueAccent;
            }
        }
    }

    Item {
        id: buttonContainer;
        anchors.top: itemName.bottom;
        anchors.topMargin: 8;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;
        anchors.left: itemPreviewImage.right;
        anchors.leftMargin: 8;
        anchors.right: parent.right;
        anchors.rightMargin: 8;

        // "Rez" button
        HifiControlsUit.Button {
            id: rezButton;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: parent.height/2 - 4;
            text: "Rez Item"
            onClicked: {
                if (urlHandler.canHandleUrl(root.itemHref)) {
                    urlHandler.handleUrl(root.itemHref);
                }
            }
        }

        // "More Info" button
        HifiControlsUit.Button {
            id: moreInfoButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: parent.height/2 - 4;
            text: "More Info"
            onClicked: {
                sendToPurchases({method: 'purchases_itemInfoClicked', itemId: root.itemId});
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToPurchases(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
