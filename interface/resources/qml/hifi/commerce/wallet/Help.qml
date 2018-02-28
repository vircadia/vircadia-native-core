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

    ListModel {
        id: helpModel;

        ListElement {
            isExpanded: false;
            question: "How can I get HFC?";
            answer: "High Fidelity commerce is in open beta right now. Want more HFC? \
Get it by going to <br><br><b><font color='#0093C5'><a href='#bank'>BankOfHighFidelity.</a></font></b> and meeting with the banker!";
        }
        ListElement {
            isExpanded: false;
            question: "What are private keys and where are they stored?";
            answer: 
                "A private key is a secret piece of text that is used to prove ownership, unlock confidential information, and sign transactions. \
In High Fidelity, your private key is used to securely access the contents of your Wallet and Purchases. \
After wallet setup, a hifikey file is stored on your computer in High Fidelity Interface's AppData directory. \
Your hifikey file contains your private key and is protected by your wallet passphrase. \
<br><br>It is very important to back up your hifikey file! \
<b><font color='#0093C5'><a href='#privateKeyPath'>Tap here to open the folder where your HifiKeys are stored on your main display.</a></font></b>"
        }
        ListElement {
            isExpanded: false;
            question: "How do I back up my private keys?";
            answer: "You can back up your hifikey file (which contains your private key and is encrypted using your wallet passphrase) by copying it to a USB flash drive, or to a service like Dropbox or Google Drive. \
Restore your hifikey file by replacing the file in Interface's AppData directory with your backup copy. \
Others with access to your back up should not be able to spend your HFC without your passphrase. \
<b><font color='#0093C5'><a href='#privateKeyPath'>Tap here to open the folder where your HifiKeys are stored on your main display.</a></font></b>";
        }
        ListElement {
            isExpanded: false;
            question: "What happens if I lose my private keys?";
            answer: "We cannot stress enough that you should keep a backup! For security reasons, High Fidelity does not keep a copy, and cannot restore it for you. \
If you lose your private key, you will no longer have access to the contents of your Wallet or My Purchases. \
Here are some things to try:<ul>\
<li>If you have backed up your hifikey file before, search your backup location</li>\
<li>Search your AppData directory in the last machine you used to set up the Wallet</li>\
<li>If you are a developer and have installed multiple builds of High Fidelity, your hifikey file might be in another folder</li>\
</ul><br><br>As a last resort, you can set up your Wallet again and generate a new hifikey file. \
Unfortunately, this means you will start with 0 HFC and your purchased items will not be transferred over.";
        }
        ListElement {
            isExpanded: false;
            question: "What if I forget my wallet passphrase?";
            answer: "Your wallet passphrase is used to encrypt your private keys. Please write it down and store it securely! \
<br><br>If you forget your passphrase, you will no longer be able to decrypt the hifikey file that the passphrase protects. \
You will also no longer have access to the contents of your Wallet or My Purchases. \
For security reasons, High Fidelity does not keep a copy of your passphrase, and can't restore it for you. \
<br><br>If you still cannot remember your wallet passphrase, you can set up your Wallet again and generate a new hifikey file. \
Unfortunately, this means you will start with 0 HFC and your purchased items will not be transferred over.";
        }
        ListElement {
            isExpanded: false;
            question: "How do I send HFC to other people?";
            answer: "You can send HFC to a High Fidelity connection (someone you've shaken hands with in-world) or somebody Nearby (currently in the same domain as you). \
In your Wallet's Send Money tab, choose from your list of connections, or choose Nearby and select the glowing sphere of the person's avatar.";
        }
        ListElement {
            isExpanded: false;
            question: "What is a Security Pic?"
            answer: "Your Security Pic is an encrypted image that you select during Wallet Setup. \
It acts as an extra layer of Wallet security. \
When you see your Security Pic, you know that your actions and data are securely making use of your private keys.\
<br><br>Don't enter your passphrase anywhere that doesn't display your Security Pic! \
If you don't see your Security Pic on a page that requests your Wallet passphrase, someone untrustworthy may be trying to access your Wallet.";
        }
        ListElement {
            isExpanded: false;
            question: "Why does my HFC balance not update instantly?";
            answer: "HFC transations sometimes takes a few seconds to update as they are backed by a blockchain. \
<br><br><b><font color='#0093C5'><a href='#blockchain'>Tap here to learn more about the blockchain.</a></font></b>";
        }
        ListElement {
            isExpanded: false;
            question: "Do I get charged money if a transaction fails?";
            answer: "<b>No.</b> Your HFC balance only changes after a transaction is confirmed.";
        }
        ListElement {
            isExpanded: false;
            question: "How do I convert HFC to other currencies?"
            answer: "We are hard at work building the tools needed to support a vibrant economy in High Fidelity. \
At the moment, there is currently no way to convert HFC to other currencies. Stay tuned...";
        }
        ListElement {
            isExpanded: false;
            question: "Who can I reach out to with questions?";
            answer: "Please email us if you have any issues or questions: \
<b><font color='#0093C5'><a href='#support'>support@highfidelity.com</a></font></b>";
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
                        } else if (link === "#bank") {
                            Qt.openUrlExternally("hifi://BankOfHighFidelity");
                        } else if (link === "#support") {
                            Qt.openUrlExternally("mailto:support@highfidelity.com");
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
