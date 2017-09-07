//
//  SendMoney.qml
//  qml/hifi/commerce/wallet
//
//  SendMoney
//
//  Created by Zach Fox on 2017-08-18
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

Item {
    HifiConstants { id: hifi; }

    id: root;

    Hifi.QmlCommerce {
        id: commerce;
    }

    // "Unavailable"
    RalewayRegular {
        id: helpText;
        text: "Help me!";
        // Anchors
        anchors.fill: parent;
        // Text size
        size: 24;
        // Style
        color: hifi.colors.faintGray;
        wrapMode: Text.WordWrap;
        // Alignment
        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;
    }
    HifiControlsUit.Button {
        color: hifi.buttons.black;
        colorScheme: hifi.colorSchemes.dark;
        anchors.bottom: resetButton.top;
        anchors.bottomMargin: 15;
        anchors.horizontalCenter: parent.horizontalCenter;
        height: 50;
        width: 250;
        text: "DEBUG: Clear Cached Passphrase";
        onClicked: {
            commerce.setPassphrase("");
            sendSignalToWallet({method: 'passphraseReset'});
        }
    }
    HifiControlsUit.Button {
        id: resetButton;
        color: hifi.buttons.red;
        colorScheme: hifi.colorSchemes.dark;
        anchors.bottom: helpText.bottom;
        anchors.bottomMargin: 15;
        anchors.horizontalCenter: parent.horizontalCenter;
        height: 50;
        width: 250;
        text: "DEBUG: Reset Wallet!";
        onClicked: {
            commerce.reset();
            sendSignalToWallet({method: 'walletReset'});
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
    // message: The message sent from the JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from a script.
    //
    function fromScript(message) {
        switch (message.method) {
            default:
                console.log('Unrecognized message from wallet.js:', JSON.stringify(message));
        }
    }
    signal sendSignalToWallet(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
