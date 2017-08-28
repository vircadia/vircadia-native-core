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
        anchors.bottom: homeMessage.visible ? homeMessage.top : root.bottom;
        anchors.bottomMargin: 10;

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
            anchors.bottom: toggleFullHistoryButton.top;
            anchors.bottomMargin: 8;
            anchors.left: parent.left;
            anchors.right: parent.right;

            // some placeholder stuff
            RalewayRegular {
                text: homeMessage.visible ? "you <b>CANNOT</b> scroll through this." : "you <b>CAN</b> scroll through this";
                // Text size
                size: 16;
                // Anchors
                anchors.fill: parent;
                // Style
                color: hifi.colors.darkGray;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
        }

        HifiControlsUit.Button {
            id: toggleFullHistoryButton;
            color: hifi.buttons.black;
            colorScheme: hifi.colorSchemes.dark;
            anchors.bottom: parent.bottom;
            anchors.right: parent.right;
            width: 250;
            height: 40;
            text: homeMessage.visible ? "See Full Transaction History" : "Collapse Transaction History";
            onClicked: {
                if (homeMessage.visible) {
                    homeMessage.visible = false;
                } else {
                    homeMessage.visible = true;
                }
            }
        }
    }

    // Item for "messages" - like "Welcome"
    Item {
        id: homeMessage;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        anchors.right: parent.right;
        anchors.rightMargin: 20;
        height: childrenRect.height;

        RalewayRegular {
            id: messageText;
            text: "<b>Welcome! Let's get you some spending money.</b><br><br>" +
            "Now that your account is all set up, click the button below to request your starter money. " +
            "A robot will promptly review your request and put money into your account.";
            // Text size
            size: 16;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 130;
            // Style
            color: hifi.colors.faintGray;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        Item {
            id: homeMessageButtons;
            anchors.top: messageText.bottom;
            anchors.topMargin: 4;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 40;
            HifiControlsUit.Button {
                id: noThanksButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                width: 100;
                text: "No Thanks"
                onClicked: {
                    messageText.text = "Okay...weird. Who doesn't like free money? If you change your mind, too bad. Sorry."
                    homeMessageButtons.visible = false;
                }
            }
            HifiControlsUit.Button {
                id: freeMoneyButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                width: 210;
                text: "Free Money Please"
                onClicked: {
                    messageText.text = "Go, MoneyRobots, Go!"
                    homeMessageButtons.visible = false;
                }
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
