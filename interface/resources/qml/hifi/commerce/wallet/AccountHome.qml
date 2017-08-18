//
//  AccountHome.qml
//  qml/hifi/commerce/wallet
//
//  AccountHome
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
    
    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            usernameText.text = Account.username;
        }
    }
    
    // Security Image
    Image {
        id: passphrasePageSecurityImage;
        // Anchors
        anchors.top: parent.top;
        anchors.left: parent.left;
        height: 75;
        width: height;
        fillMode: Image.PreserveAspectFit;
        mipmap: true;
    }

    // Username Text
    RalewayRegular {
        id: usernameText;
        text: Account.username;
        // Text size
        size: 24;
        // Style
        color: hifi.colors.faintGray;
        elide: Text.ElideRight;
        // Anchors
        anchors.top: passphrasePageSecurityImage.top;
        anchors.bottom: passphrasePageSecurityImage.bottom;
        anchors.left: passphrasePageSecurityImage.right;
        anchors.leftMargin: 16;
        anchors.right: hfcBalanceContainer.left;
    }

    Rectangle {
        id: hfcBalanceContainer;
        anchors.right: parent.right;
        anchors.verticalCenter: passphrasePageSecurityImage.verticalCenter;
        width: 175;
        height: 45;
    }

    //
    // FUNCTION DEFINITIONS START
    //
    function setSecurityImage(imagePath) {
        passphrasePageSecurityImage.source = imagePath;
    }
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
