//
//  NotSetUp.qml
//  qml/hifi/commerce/wallet
//
//  NotSetUp
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

    //
    // TAB CONTENTS START
    //

    // Text below title bar
    RalewaySemiBold {
        id: notSetUpText;
        text: "Your Wallet Account Has Not Been Set Up";
        // Text size
        size: 22;
        // Anchors
        anchors.top: parent.top;
        anchors.topMargin: 100;
        anchors.left: parent.left;
        anchors.leftMargin: 16;
        anchors.right: parent.right;
        anchors.rightMargin: 16;
        height: 50;
        width: paintedWidth;
        // Style
        color: hifi.colors.faintGray;
        wrapMode: Text.WordWrap;
        // Alignment
        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;
    }

    // Explanitory text
    RalewayRegular {
        text: "To buy and sell items in High Fidelity Coin (HFC), you first need " +
        "to set up your wallet.<br><b>You do not need to submit a credit card or personal information to set up your wallet.</b>";
        // Text size
        size: 18;
        // Anchors
        anchors.top: notSetUpText.bottom;
        anchors.topMargin: 16;
        anchors.left: parent.left;
        anchors.leftMargin: 30;
        anchors.right: parent.right;
        anchors.rightMargin: 30;
        height: 100;
        width: paintedWidth;
        // Style
        color: hifi.colors.faintGray;
        wrapMode: Text.WordWrap;
        // Alignment
        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;
    }

    // "Set Up" button
    HifiControlsUit.Button {
        color: hifi.buttons.blue;
        colorScheme: hifi.colorSchemes.dark;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 150;
        anchors.horizontalCenter: parent.horizontalCenter;
        width: parent.width/2;
        height: 50;
        text: "Set Up My Wallet";
        onClicked: {
            sendSignalToWallet({method: 'setUpClicked'});
        }
    }


    //
    // TAB CONTENTS END
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
