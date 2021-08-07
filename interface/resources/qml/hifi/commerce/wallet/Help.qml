//
//  Help.qml
//  qml/hifi/commerce/wallet
//
//  Help
//
//  Created by Zach Fox on 2017-08-18
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.7
import QtQuick.Controls 2.2
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
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
Get it by going to <b><font color='#0093C5'><a href='#bank'>BankOfHighFidelity</a></font></b> and meeting with the banker!";
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
            answer: "Your Security Pic acts as an extra layer of Wallet security. \
When you see your Security Pic, you know that your actions and data are securely making use of your account.";
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
                            Qt.openUrlExternally("https://docs.vircadia.com/explore/shop.html");
                        } else if (link === "#bank") {
                            if ((Account.metaverseServerURL).toString().indexOf("staging") >= 0) {
                                Qt.openUrlExternally("hifi://hifiqa-master-metaverse-staging"); // So that we can test in staging.
                            } else {
                                Qt.openUrlExternally("hifi://BankOfHighFidelity");
                            }
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
                console.log('Help.qml: Unrecognized message from wallet.js');
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
