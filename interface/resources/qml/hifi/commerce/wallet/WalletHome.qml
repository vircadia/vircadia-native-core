//
//  WalletHome.qml
//  qml/hifi/commerce/wallet
//
//  WalletHome
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

        onSecurityImageResult: {
            if (exists) {
                // just set the source again (to be sure the change was noticed)
                securityImage.source = "";
                securityImage.source = "image://security/securityImage";
            }
        }

        onBalanceResult : {
            balanceText.text = parseFloat(result.data.balance/100).toFixed(2);
        }

        onHistoryResult : {
            if (result.status === 'success') {
                var txt = result.data.history.map(function (h) { return h.text; }).join("<hr>");
                transactionHistoryText.text = txt;
            }
        }
    }

    SecurityImageModel {
        id: securityImageModel;
    }

    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            usernameText.text = Account.username;
        }
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
        anchors.top: securityImageContainer.top;
        anchors.bottom: securityImageContainer.bottom;
        anchors.left: parent.left;
        anchors.right: hfcBalanceContainer.left;
    }

    // HFC Balance Container
    Item {
        id: hfcBalanceContainer;
        // Anchors
        anchors.top: securityImageContainer.top;
        anchors.right: securityImageContainer.left;
        anchors.rightMargin: 16;
        width: 175;
        height: 60;
        Rectangle {
            id: hfcBalanceField;
            anchors.right: parent.right;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            height: parent.height - 15;

            // "HFC" balance label
            RalewayRegular {
                id: balanceLabel;
                text: "HFC";
                // Text size
                size: 20;
                // Anchors
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: hfcBalanceField.right;
                anchors.rightMargin: 4;
                width: paintedWidth;
                // Style
                color: hifi.colors.darkGray;
                // Alignment
                horizontalAlignment: Text.AlignRight;
                verticalAlignment: Text.AlignVCenter;

                onVisibleChanged: {
                    if (visible) {
                        commerce.balance();
                        commerce.history();
                    }
                }
            }

            // Balance Text
            FiraSansRegular {
                id: balanceText;
                text: "--";
                // Text size
                size: 28;
                // Anchors
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                anchors.right: balanceLabel.left;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.darkGray;
                // Alignment
                horizontalAlignment: Text.AlignRight;
                verticalAlignment: Text.AlignVCenter;
            }
        }
        // "balance" text above field
        RalewayRegular {
            text: "balance";
            // Text size
            size: 12;
            // Anchors
            anchors.top: parent.top;
            anchors.bottom: hfcBalanceField.top;
            anchors.bottomMargin: 4;
            anchors.left: hfcBalanceField.left;
            anchors.right: hfcBalanceField.right;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter;
        }
    }

    // Security Image
    Item {
        id: securityImageContainer;
        // Anchors
        anchors.top: parent.top;
        anchors.right: parent.right;
        width: 75;
        height: childrenRect.height;

        onVisibleChanged: {
            if (visible) {
                commerce.getSecurityImage();
            }
        }

        Image {
            id: securityImage;
            // Anchors
            anchors.top: parent.top;
            anchors.horizontalCenter: parent.horizontalCenter;
            height: parent.width - 10;
            width: height;
            fillMode: Image.PreserveAspectFit;
            mipmap: true;
            cache: false;
            source: "image://security/securityImage";
        }
        // "Security picture" text below pic
        RalewayRegular {
            text: "security picture";
            // Text size
            size: 12;
            // Anchors
            anchors.top: securityImage.bottom;
            anchors.topMargin: 4;
            anchors.left: securityImageContainer.left;
            anchors.right: securityImageContainer.right;
            height: paintedHeight;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }
    }

    // Recent Activity
    Item {
        id: recentActivityContainer;
        anchors.top: securityImageContainer.bottom;
        anchors.topMargin: 8;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        RalewayRegular {
            id: recentActivityText;
            text: "Recent Activity";
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 30;
            // Text size
            size: 22;
            // Style
            color: hifi.colors.faintGray;
        }

        Rectangle {
            id: transactionHistory;
            anchors.top: recentActivityText.bottom;
            anchors.topMargin: 4;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;

            // some placeholder stuff
            TextArea {
                id: transactionHistoryText;
                text: "<hr>history unavailable<hr>";
                textFormat: TextEdit.AutoText;
                font.pointSize: 10;
                anchors.fill: parent;
                horizontalAlignment: Text.AlignLeft;
                verticalAlignment: Text.AlignTop;
            }
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
