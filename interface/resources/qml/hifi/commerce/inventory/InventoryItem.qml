//
//  InventoryItem.qml
//  qml/hifi/commerce/inventory
//
//  InventoryItem
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
    // Style
    color: hifi.colors.baseGray;

    
    RalewayRegular {
        id: thisItemId;
        // Text size
        size: 20;
        // Style
        color: hifi.colors.blueAccent;
        text: itemName;
        // Alignment
        horizontalAlignment: Text.AlignLeft;
    }
    MouseArea {
        anchors.fill: parent;
        hoverEnabled: enabled;
        onClicked: {
            sendToScript({method: 'inventory_itemClicked', itemId: itemId});
        }
        onEntered: {
            thisItemId.color = hifi.colors.blueHighlight;
        }
        onExited: {
            thisItemId.color = hifi.colors.blueAccent;
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
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
