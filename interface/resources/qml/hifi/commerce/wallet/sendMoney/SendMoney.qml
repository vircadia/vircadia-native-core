//
//  SendMoney.qml
//  qml/hifi/commerce/wallet/sendMoney
//
//  SendMoney
//
//  Created by Zach Fox on 2018-01-09
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 2.2
import "../../../../styles-uit"
import "../../../../controls-uit" as HifiControlsUit
import "../../../../controls" as HifiControls
import "../../common" as HifiCommerceCommon

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;

    property int parentAppTitleBarHeight;
    property int parentAppNavBarHeight;
    property string activeView: "sendMoneyHome";
    property bool isCurrentlyFullScreen: chooseRecipientConnection.visible;
        
    // This object is always used in a popup or full-screen Wallet section.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup/section.
    MouseArea {
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        y: root.isCurrentlyFullScreen ? root.parentAppTitleBarHeight : 0;
        height: root.isCurrentlyFullScreen ? parent.height : parent.height - root.parentAppTitleBarHeight - root.parentAppNavBarHeight;
        propagateComposedEvents: false;
    }

    Connections {
        target: Commerce;

        onBalanceResult : {
            balanceText.text = result.data.balance;
        }
    }

    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            transactionHistoryModel.clear();
            usernameText.text = Account.username;
        }
    }

    // Send Money Home BEGIN
    Item {
        id: sendMoneyHome;
        visible: root.activeView === "sendMoneyHome";
        anchors.fill: parent;
        anchors.topMargin: root.parentAppTitleBarHeight;
        anchors.bottomMargin: root.parentAppNavBarHeight;

        // Username Text
        RalewayRegular {
            id: usernameText;
            text: Account.username;
            // Text size
            size: 24;
            // Style
            color: hifi.colors.white;
            elide: Text.ElideRight;
            // Anchors
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: parent.width/2;
            height: 80;
        }

        // HFC Balance Container
        Item {
            id: hfcBalanceContainer;
            // Anchors
            anchors.top: parent.top;
            anchors.right: parent.right;
            anchors.leftMargin: 20;
            width: parent.width/2;
            height: 80;
        
            // "HFC" balance label
            HiFiGlyphs {
                id: balanceLabel;
                text: hifi.glyphs.hfc;
                // Size
                size: 40;
                // Anchors
                anchors.left: parent.left;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                // Style
                color: hifi.colors.white;
            }

            // Balance Text
            FiraSansRegular {
                id: balanceText;
                text: "--";
                // Text size
                size: 28;
                // Anchors
                anchors.top: balanceLabel.top;
                anchors.bottom: balanceLabel.bottom;
                anchors.left: balanceLabel.right;
                anchors.leftMargin: 10;
                anchors.right: parent.right;
                anchors.rightMargin: 4;
                // Style
                color: hifi.colors.white;
                // Alignment
                verticalAlignment: Text.AlignVCenter;

                onVisibleChanged: {
                    if (visible) {
                        Commerce.balance();
                    }
                }
            }

            // "balance" text below field
            RalewayRegular {
                text: "BALANCE (HFC)";
                // Text size
                size: 14;
                // Anchors
                anchors.top: balanceLabel.top;
                anchors.topMargin: balanceText.paintedHeight + 20;
                anchors.bottom: balanceLabel.bottom;
                anchors.left: balanceText.left;
                anchors.right: balanceText.right;
                height: paintedHeight;
                // Style
                color: hifi.colors.white;
            }
        }

        // Send Money
        Rectangle {
            id: sendMoneyContainer;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
            height: 440;

            RalewaySemiBold {
                id: sendMoneyText;
                text: "Send Money To:";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 26;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: paintedWidth;
                height: 30;
                // Text size
                size: 22;
                // Style
                color: hifi.colors.baseGray;
            }

            Item {
                id: connectionButton;
                // Anchors
                anchors.top: sendMoneyText.bottom;
                anchors.topMargin: 40;
                anchors.left: parent.left;
                anchors.leftMargin: 75;
                height: 95;
                width: 95;

                Image {
                    anchors.top: parent.top;
                    source: "../images/wallet-bg.jpg";
                    height: 60;
                    width: parent.width;
                    fillMode: Image.PreserveAspectFit;
                    horizontalAlignment: Image.AlignHCenter;
                    verticalAlignment: Image.AlignTop;
                }

                RalewaySemiBold {
                    text: "Connection";
                    // Anchors
                    anchors.bottom: parent.bottom;
                    height: 15;
                    width: parent.width;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    horizontalAlignment: Text.AlignHCenter;
                }

                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        root.activeView = "chooseRecipientConnection";
                    }
                }
            }

            Item {
                id: nearbyButton;
                // Anchors
                anchors.top: sendMoneyText.bottom;
                anchors.topMargin: connectionButton.anchors.topMargin;
                anchors.right: parent.right;
                anchors.rightMargin: connectionButton.anchors.leftMargin;
                height: connectionButton.height;
                width: connectionButton.width;

                Image {
                    anchors.top: parent.top;
                    source: "../images/wallet-bg.jpg";
                    height: 60;
                    width: parent.width;
                    fillMode: Image.PreserveAspectFit;
                    horizontalAlignment: Image.AlignHCenter;
                    verticalAlignment: Image.AlignTop;
                }

                RalewaySemiBold {
                    text: "Someone Nearby";
                    // Anchors
                    anchors.bottom: parent.bottom;
                    height: 15;
                    width: parent.width;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    horizontalAlignment: Text.AlignHCenter;
                }

                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        root.activeView = "chooseRecipientNearby";
                    }
                }
            }
        }
    }
    // Send Money Home END
    
    // Choose Recipient Connection BEGIN
    Rectangle {
        id: chooseRecipientConnection;
        visible: root.activeView === "chooseRecipientConnection";
        anchors.fill: parent;
        color: "#AAAAAA";

        onVisibleChanged: {
            if (visible) {
                // Refresh connections model
                connectionsLoading.visible = false;
                connectionsLoading.visible = true;
                sendSignalToWallet({method: 'refreshConnections'});
            }
        }
        
        ListModel {
            id: connectionsModel;
        }
        ListModel {
            id: filteredConnectionsModel;
        }

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;

            RalewaySemiBold {
                id: chooseRecipientText;
                text: "Choose Recipient:";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 26;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: paintedWidth;
                height: 30;
                // Text size
                size: 22;
                // Style
                color: hifi.colors.baseGray;
            }
            
            HiFiGlyphs {
                id: closeGlyphButton;
                text: hifi.glyphs.close;
                size: 26;
                anchors.top: parent.top;
                anchors.topMargin: 10;
                anchors.right: parent.right;
                anchors.rightMargin: 10;
                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onEntered: {
                        parent.text = hifi.glyphs.closeInverted;
                    }
                    onExited: {
                        parent.text = hifi.glyphs.close;
                    }
                    onClicked: {
                        root.activeView = "sendMoneyHome";
                    }
                }
            }

            //
            // FILTER BAR START
            //
            Item {
                id: filterBarContainer;
                // Size
                height: 40;
                // Anchors
                anchors.left: parent.left;
                anchors.leftMargin: 16;
                anchors.right: parent.right;
                anchors.rightMargin: 16;
                anchors.top: chooseRecipientText.bottom;
                anchors.topMargin: 12;

                HifiControlsUit.TextField {
                    id: filterBar;
                    colorScheme: hifi.colorSchemes.faintGray;
                    hasClearButton: true;
                    hasRoundedBorder: true;
                    anchors.fill: parent;
                    placeholderText: "filter recipients";

                    onTextChanged: {
                        buildFilteredConnectionsModel();
                    }

                    onAccepted: {
                        focus = false;
                    }
                }
            }
            //
            // FILTER BAR END
            //

            Item {
                id: connectionsContainer;
                anchors.top: filterBarContainer.bottom;
                anchors.topMargin: 16;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.bottom: parent.bottom;
                
                AnimatedImage {
                    id: connectionsLoading;
                    source: "../../../../../icons/profilePicLoading.gif"
                    width: 120;
                    height: width;
                    anchors.top: parent.top;
                    anchors.topMargin: 185;
                    anchors.horizontalCenter: parent.horizontalCenter;
                }

                ListView {
                    id: connectionsList;
                    ScrollBar.vertical: ScrollBar {
                        policy: connectionsList.contentHeight > parent.parent.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded;
                        parent: connectionsList.parent;
                        anchors.top: connectionsList.top;
                        anchors.right: connectionsList.right;
                        anchors.bottom: connectionsList.bottom;
                        width: 20;
                    }
                    visible: !connectionsLoading.visible;
                    clip: true;
                    model: filteredConnectionsModel;
                    snapMode: ListView.SnapToItem;
                    // Anchors
                    anchors.fill: parent;
                    delegate: ConnectionItem {
                        isSelected: connectionsList.currentIndex === index;
                        userName: model.userName;
                        profilePicUrl: model.profileUrl;
                        anchors.topMargin: 6;
                        anchors.bottomMargin: 6;

                        Connections {
                            onSendToSendMoney: {
                                // TODO
                            }
                        }

                        MouseArea {
                            enabled: connectionsList.currentIndex !== index;
                            anchors.fill: parent;
                            propagateComposedEvents: false;
                            onClicked: {
                                connectionsList.currentIndex = index;
                            }
                        }
                    }
                }
            }
        }
    }
    // Choose Recipient Connection END



    //
    // FUNCTION DEFINITIONS START
    //

    function updateConnections(connections) {
        connectionsModel.clear();
        connectionsModel.append(connections);
        buildFilteredConnectionsModel();
        connectionsLoading.visible = false;
    }

    function buildFilteredConnectionsModel() {
        filteredConnectionsModel.clear();
        for (var i = 0; i < connectionsModel.count; i++) {
            if (connectionsModel.get(i).userName.toLowerCase().indexOf(filterBar.text.toLowerCase()) !== -1) {
                filteredConnectionsModel.append(connectionsModel.get(i));
            }
        }
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
