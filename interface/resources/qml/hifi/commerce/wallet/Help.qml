//
//  Help.qml
//  qml/hifi/commerce/wallet
//
//  Help
//
//  Created by Zach Fox on 2017-08-18
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.7
import QtQuick.Controls 2.2
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    property string keyFilePath;
    property bool showDebugButtons: true;

    Connections {
        target: Commerce;

        onKeyFilePathIfExistsResult: {
            root.keyFilePath = path;
        }
    }

    onVisibleChanged: {
        if (visible) {
            Commerce.getKeyFilePathIfExists();
        }
    }

    RalewaySemiBold {
        id: helpTitleText;
        text: "Help Topics";
        // Anchors
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        width: paintedWidth;
        height: 30;
        // Text size
        size: 18;
        // Style
        color: hifi.colors.blueHighlight;
    }
    HifiControlsUit.Button {
        id: clearCachedPassphraseButton;
        visible: root.showDebugButtons;
        color: hifi.buttons.black;
        colorScheme: hifi.colorSchemes.dark;
        anchors.top: parent.top;
        anchors.left: helpTitleText.right;
        anchors.leftMargin: 20;
        height: 40;
        width: 150;
        text: "DBG: Clear Pass";
        onClicked: {
            Commerce.setPassphrase("");
            sendSignalToWallet({method: 'passphraseReset'});
        }
    }
    HifiControlsUit.Button {
        id: resetButton;
        visible: root.showDebugButtons;
        color: hifi.buttons.red;
        colorScheme: hifi.colorSchemes.dark;
        anchors.top: clearCachedPassphraseButton.top;
        anchors.left: clearCachedPassphraseButton.right;
        height: 40;
        width: 150;
        text: "DBG: RST Wallet";
        onClicked: {
            Commerce.reset();
            sendSignalToWallet({method: 'walletReset'});
        }
    }

    ListModel {
        id: helpModel;

        ListElement {
            isExpanded: false;
            question: "How can I get HFC?"
            answer: qsTr("High Fidelity commerce is in closed beta right now.<br><br>To request entry and get free HFC, <b>please contact info@highfidelity.com with your High Fidelity account username and the email address registered to that account.</b>");
        }
        ListElement {
            isExpanded: false;
            question: "What are private keys?"
            answer: qsTr("A private key is a secret piece of text that is used to prove ownership, unlock confidential information, and sign transactions.<br><br>In High Fidelity, <b>your private keys are used to securely access the contents of your Wallet and Purchases.</b>");
        }
        ListElement {
            isExpanded: false;
            question: "Where are my private keys stored?"
            answer: qsTr('By default, your private keys are <b>only stored on your hard drive</b> in High Fidelity Interface\'s AppData directory.<br><br><b><font color="#0093C5"><a href="#privateKeyPath">Tap here to open the folder where your HifiKeys are stored on your main display.</a></font></b>');
        }
        ListElement {
            isExpanded: false;
            question: "How can I backup my private keys?"
            answer: qsTr('You may backup the file containing your private keys by copying it to a USB flash drive, or to a service like Dropbox or Google Drive.<br><br>Restore your backup by replacing the file in Interface\'s AppData directory with your backed-up copy.<br><br><b><font color="#0093C5"><a href="#privateKeyPath">Tap here to open the folder where your HifiKeys are stored on your main display.</a></font></b>');
        }
        ListElement {
            isExpanded: false;
            question: "What happens if I lose my passphrase?"
            answer: qsTr("Your passphrase is used to encrypt your private keys. If you lose your passphrase, you will no longer be able to decrypt your private key file. You will also no longer have access to the contents of your Wallet or My Purchases.<br><br><b>Nobody can help you recover your passphrase, including High Fidelity.</b> Please write it down and store it securely.");
        }
        ListElement {
            isExpanded: false;
            question: "What is a 'Security Pic'?"
            answer: qsTr("Your Security Pic is an encrypted image that you selected during Wallet Setup. <b>It acts as an extra layer of Wallet security.</b><br><br>When you see your Security Pic, you know that your actions and data are securely making use of your private keys.<br><br><b>If you don't see your Security Pic on a page that is asking you for your Wallet passphrase, someone untrustworthy may be trying to gain access to your Wallet.</b><br><br>The encrypted Pic is stored on your hard drive inside the same file as your private keys.");
        }
        ListElement {
            isExpanded: false;
            question: "My HFC balance isn't what I expect it to be. Why?"
            answer: qsTr('High Fidelity Coin (HFC) transactions are backed by a <b>blockchain</b>, which takes time to update. The status of a transaction usually updates within a few seconds.<br><br><b><font color="#0093C5"><a href="#blockchain">Tap here to learn more about the blockchain.</a></font></b>');
        }
        ListElement {
            isExpanded: false;
            question: "Do I get charged money if a transaction fails?"
            answer: qsTr("<b>No.</b> Your HFC balance only changes after a transaction is confirmed.");
        }
        ListElement {
            isExpanded: false;
            question: "How do I convert HFC to other currencies?"
            answer: qsTr("We are still building the tools needed to support a vibrant economy in High Fidelity. <b>There is currently no way to convert HFC to other currencies.</b>");
        }
    }

    ListView {
        id: helpListView;
        ScrollBar.vertical: ScrollBar {
        policy: helpListView.contentHeight > helpListView.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded;
        parent: helpListView.parent;
        anchors.top: helpListView.top;
        anchors.right: helpListView.right;
        anchors.bottom: helpListView.bottom;
        width: 20;
        }
        anchors.top: helpTitleText.bottom;
        anchors.topMargin: 30;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right
        clip: true;
        model: helpModel;
        delegate: Item {
            width: parent.width;
            height: model.isExpanded ? questionContainer.height + answerContainer.height : questionContainer.height;

            HifiControlsUit.Separator {
            colorScheme: 1;
            visible: index === 0;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: parent.top;
            }

            Item {
                id: questionContainer;
                anchors.top: parent.top;
                anchors.left: parent.left;
                width: parent.width;
                height: questionText.paintedHeight + 50;
            
                RalewaySemiBold {
                    id: plusMinusButton;
                    text: model.isExpanded ? "-" : "+";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.topMargin: model.isExpanded ? -9 : 0;
                    anchors.bottom: parent.bottom;
                    anchors.left: parent.left;
                    width: 60;
                    // Text size
                    size: 60;
                    // Style
                    color: hifi.colors.white;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                }

                RalewaySemiBold {
                    id: questionText;
                    text: model.question;
                    size: 18;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: plusMinusButton.right;
                    anchors.leftMargin: 4;
                    anchors.right: parent.right;
                    anchors.rightMargin: 10;
                    wrapMode: Text.WordWrap;
                    height: paintedHeight;
                    color: hifi.colors.white;
                    verticalAlignment: Text.AlignVCenter;
                }

                MouseArea {
                    id: securityTabMouseArea;
                    anchors.fill: parent;
                    onClicked: {
                        model.isExpanded = !model.isExpanded;
                        if (model.isExpanded) {
                            collapseAllOtherHelpItems(index);
                        }
                    }
                }
            }

            Rectangle {
                id: answerContainer;
                visible: model.isExpanded;
                color: Qt.rgba(0, 0, 0, 0.5);
                anchors.top: questionContainer.bottom;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: answerText.paintedHeight + 50;

                RalewayRegular {
                    id: answerText;
                    text: model.answer;
                    size: 18;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: parent.left;
                    anchors.leftMargin: 32;
                    anchors.right: parent.right;
                    anchors.rightMargin: 32;
                    wrapMode: Text.WordWrap;
                    height: paintedHeight;
                    color: hifi.colors.white;

                    onLinkActivated: {
                        if (link === "#privateKeyPath") {
                            Qt.openUrlExternally("file:///" + root.keyFilePath.substring(0, root.keyFilePath.lastIndexOf('/')));
                        } else if (link === "#blockchain") {
                            Qt.openUrlExternally("https://docs.highfidelity.com/high-fidelity-commerce");
                        }
                    }
                }
            }

            HifiControlsUit.Separator {
            colorScheme: 1;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.bottom: parent.bottom;
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

    function collapseAllOtherHelpItems(thisIndex) {
        for (var i = 0; i < helpModel.count; i++) {
            if (i !== thisIndex) {
                helpModel.setProperty(i, "isExpanded", false);
            }
        }
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
