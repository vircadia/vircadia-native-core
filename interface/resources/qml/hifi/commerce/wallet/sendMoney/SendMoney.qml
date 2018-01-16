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
import QtQuick 2.6
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "../../../../styles-uit"
import "../../../../controls-uit" as HifiControlsUit
import "../../../../controls" as HifiControls
import "../../common" as HifiCommerceCommon

Item {
    HifiConstants { id: hifi; }

    id: root;

    property int parentAppTitleBarHeight;
    property int parentAppNavBarHeight;
    property string currentActiveView: "sendMoneyHome";
    property string nextActiveView: "";
    property bool isCurrentlyFullScreen: chooseRecipientConnection.visible ||
        chooseRecipientNearby.visible || sendMoneyStep.visible || paymentSuccess.visible || paymentFailure.visible;
    property bool isCurrentlySendingMoney: false;
        
    // This object is always used in a popup or full-screen Wallet section.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup/section.
    MouseArea {
        x: 0;
        y: root.isCurrentlyFullScreen ? 0 : root.parentAppTitleBarHeight;
        width: parent.width;
        height: root.isCurrentlyFullScreen ? parent.height : parent.height - root.parentAppTitleBarHeight - root.parentAppNavBarHeight;
        propagateComposedEvents: false;
    }

    Connections {
        target: Commerce;

        onBalanceResult : {
            balanceText.text = result.data.balance;
        }

        onTransferHfcToNodeResult: {
            root.isCurrentlySendingMoney = false;

            if (result.status === 'success') {
                root.nextActiveView = 'paymentSuccess';
            } else {
                root.nextActiveView = 'paymentFailure';
            }
        }

        onTransferHfcToUsernameResult: {
            root.isCurrentlySendingMoney = false;

            if (result.status === 'success') {
                root.nextActiveView = 'paymentSuccess';
            } else {
                root.nextActiveView = 'paymentFailure';
            }
        }
    }

    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            usernameText.text = Account.username;
        }
    }

    Component.onCompleted: {
        Commerce.balance();
    }

    onNextActiveViewChanged: {
        if (root.currentActiveView === 'chooseRecipientNearby') {
            sendSignalToWallet({method: 'disable_ChooseRecipientNearbyMode'});
        }

        root.currentActiveView = root.nextActiveView;

        if (root.currentActiveView === 'chooseRecipientConnection') {
            // Refresh connections model
            connectionsLoading.visible = false;
            connectionsLoading.visible = true;
            sendSignalToWallet({method: 'refreshConnections'});
        } else if (root.currentActiveView === 'sendMoneyHome') {
            Commerce.balance();
        } else if (root.currentActiveView === 'chooseRecipientNearby') {
            sendSignalToWallet({method: 'enable_ChooseRecipientNearbyMode'});
        }
    }

    // Send Money Home BEGIN
    Item {
        id: sendMoneyHome;
        visible: root.currentActiveView === "sendMoneyHome";
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
                        root.nextActiveView = "chooseRecipientConnection";
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
                        root.nextActiveView = "chooseRecipientNearby";
                    }
                }
            }
        }
    }
    // Send Money Home END
    
    // Choose Recipient Connection BEGIN
    Rectangle {
        id: chooseRecipientConnection;
        visible: root.currentActiveView === "chooseRecipientConnection";
        anchors.fill: parent;
        color: "#AAAAAA";
        
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
                id: chooseRecipientText_connections;
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
                id: closeGlyphButton_connections;
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
                        root.nextActiveView = "sendMoneyHome";
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
                anchors.top: chooseRecipientText_connections.bottom;
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
                                sendMoneyStep.referrer = "connections";
                                sendMoneyStep.selectedRecipientNodeID = '';
                                sendMoneyStep.selectedRecipientDisplayName = 'connection';
                                sendMoneyStep.selectedRecipientUserName = msg.userName;
                                sendMoneyStep.selectedRecipientProfilePic = msg.profilePicUrl;

                                root.nextActiveView = "sendMoneyStep";
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
    
    // Choose Recipient Nearby BEGIN
    Rectangle {
        id: chooseRecipientNearby;

        property string selectedRecipient;

        visible: root.currentActiveView === "chooseRecipientNearby";
        anchors.fill: parent;
        color: "#AAAAAA";

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;

            RalewaySemiBold {
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
                id: closeGlyphButton_nearby;
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
                        root.nextActiveView = "sendMoneyHome";
                        resetSendMoneyData();
                    }
                }
            }

            Item {
                id: selectionInstructionsContainer;
                visible: chooseRecipientNearby.selectedRecipient === "";
                anchors.fill: parent;

                RalewaySemiBold {
                    id: selectionInstructions_deselected;
                    text: "Click/trigger on an avatar nearby to select them...";
                    // Anchors
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: 200;
                    anchors.left: parent.left;
                    anchors.leftMargin: 58;
                    anchors.right: parent.right;
                    anchors.rightMargin: anchors.leftMargin;
                    height: paintedHeight;
                    // Text size
                    size: 20;
                    // Style
                    color: hifi.colors.baseGray;
                    horizontalAlignment: Text.AlignHCenter;
                    wrapMode: Text.Wrap;
                }
            }

            Item {
                id: selectionMadeContainer;
                visible: !selectionInstructionsContainer.visible;
                anchors.fill: parent;

                RalewaySemiBold {
                    id: sendToText;
                    text: "Send To:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.topMargin: 120;
                    anchors.left: parent.left;
                    anchors.leftMargin: 12;
                    width: paintedWidth;
                    height: paintedHeight;
                    // Text size
                    size: 20;
                    // Style
                    color: hifi.colors.baseGray;
                }

                RalewaySemiBold {
                    id: avatarDisplayName;
                    text: '"' + AvatarList.getAvatar(chooseRecipientNearby.selectedRecipient).sessionDisplayName + '"';
                    // Anchors
                    anchors.top: sendToText.bottom;
                    anchors.topMargin: 60;
                    anchors.left: parent.left;
                    anchors.leftMargin: 30;
                    anchors.right: parent.right;
                    anchors.rightMargin: 30;
                    height: paintedHeight;
                    // Text size
                    size: 22;
                    // Style
                    horizontalAlignment: Text.AlignHCenter;
                    color: hifi.colors.baseGray;
                }

                RalewaySemiBold {
                    id: avatarNodeID;
                    text: chooseRecipientNearby.selectedRecipient;
                    // Anchors
                    anchors.top: avatarDisplayName.bottom;
                    anchors.topMargin: 6;
                    anchors.left: parent.left;
                    anchors.leftMargin: 30;
                    anchors.right: parent.right;
                    anchors.rightMargin: 30;
                    height: paintedHeight;
                    // Text size
                    size: 14;
                    // Style
                    horizontalAlignment: Text.AlignHCenter;
                    color: hifi.colors.lightGrayText;
                }

                RalewaySemiBold {
                    id: avatarUserName;
                    text: sendMoneyStep.selectedRecipientUserName;
                    // Anchors
                    anchors.top: avatarNodeID.bottom;
                    anchors.topMargin: 12;
                    anchors.left: parent.left;
                    anchors.leftMargin: 30;
                    anchors.right: parent.right;
                    anchors.rightMargin: 30;
                    height: paintedHeight;
                    // Text size
                    size: 22;
                    // Style
                    horizontalAlignment: Text.AlignHCenter;
                    color: hifi.colors.baseGray;
                }

                RalewaySemiBold {
                    id: selectionInstructions_selected;
                    text: "Click/trigger on another avatar nearby to select them...\n\nor press 'Next' to continue.";
                    // Anchors
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: 200;
                    anchors.left: parent.left;
                    anchors.leftMargin: 58;
                    anchors.right: parent.right;
                    anchors.rightMargin: anchors.leftMargin;
                    height: paintedHeight;
                    // Text size
                    size: 20;
                    // Style
                    color: hifi.colors.baseGray;
                    horizontalAlignment: Text.AlignHCenter;
                    wrapMode: Text.Wrap;
                }
            }

            // "Cancel" button
            HifiControlsUit.Button {
                id: cancelButton;
                color: hifi.buttons.noneBorderless;
                colorScheme: hifi.colorSchemes.dark;
                anchors.left: parent.left;
                anchors.leftMargin: 60;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 80;
                height: 50;
                width: 120;
                text: "Cancel";
                onClicked: {
                    root.nextActiveView = "sendMoneyHome";
                    resetSendMoneyData();
                }
            }

            // "Next" button
            HifiControlsUit.Button {
                id: nextButton;
                enabled: chooseRecipientNearby.selectedRecipient !== "";
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: parent.right;
                anchors.rightMargin: 60;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 80;
                height: 50;
                width: 120;
                text: "Next";
                onClicked: {
                    sendMoneyStep.referrer = "nearby";
                    sendMoneyStep.selectedRecipientNodeID = chooseRecipientNearby.selectedRecipient;
                    chooseRecipientNearby.selectedRecipient = "";

                    root.nextActiveView = "sendMoneyStep";
                }
            }
        }
    }
    // Choose Recipient Nearby END
    
    // Send Money Screen BEGIN
    Rectangle {
        id: sendMoneyStep;
        z: 997;

        property string referrer; // either "connections" or "nearby"
        property string selectedRecipientNodeID;
        property string selectedRecipientDisplayName;
        property string selectedRecipientUserName;
        property string selectedRecipientProfilePic;

        visible: root.currentActiveView === "sendMoneyStep";
        anchors.fill: parent;
        color: "#AAAAAA";

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;

            RalewaySemiBold {
                id: sendMoneyText_sendMoneyStep;
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
                id: sendToContainer;
                anchors.top: sendMoneyText_sendMoneyStep.bottom;
                anchors.topMargin: 20;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;

                RalewaySemiBold {
                    id: sendToText_sendMoneyStep;
                    text: "Send To:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: parent.left;
                    anchors.bottom: parent.bottom;
                    width: 90;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignVCenter;
                }

                RecipientDisplay {
                    anchors.top: parent.top;
                    anchors.left: sendToText_sendMoneyStep.right;
                    anchors.right: changeButton.left;
                    anchors.rightMargin: 12;
                    height: parent.height;

                    displayName: sendMoneyStep.selectedRecipientDisplayName;
                    userName: sendMoneyStep.selectedRecipientUserName;
                    profilePic: sendMoneyStep.selectedRecipientProfilePic !== "" ? ((0 === sendMoneyStep.selectedRecipientProfilePic.indexOf("http")) ?
                        sendMoneyStep.selectedRecipientProfilePic : (Account.metaverseServerURL + sendMoneyStep.selectedRecipientProfilePic)) : "";
                    isDisplayingNearby: sendMoneyStep.referrer === "nearby";
                }

                // "CHANGE" button
                HifiControlsUit.Button {
                    id: changeButton;
                    color: hifi.buttons.black;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.right: parent.right;
                    anchors.verticalCenter: parent.verticalCenter;
                    height: 35;
                    width: 120;
                    text: "CHANGE";
                    onClicked: {
                        if (sendMoneyStep.referrer === "connections") {
                            root.nextActiveView = "chooseRecipientConnection";
                        } else if (sendMoneyStep.referrer === "nearby") {
                            root.nextActiveView = "chooseRecipientNearby";
                        }
                        resetSendMoneyData();
                    }
                }
            }

            Item {
                id: amountContainer;
                anchors.top: sendToContainer.bottom;
                anchors.topMargin: 2;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;

                RalewaySemiBold {
                    id: amountText;
                    text: "Amount:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: parent.left;
                    anchors.bottom: parent.bottom;
                    width: 90;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignVCenter;
                }

                HifiControlsUit.TextField {
                    id: amountTextField;
                    colorScheme: hifi.colorSchemes.light;
                    inputMethodHints: Qt.ImhDigitsOnly;
                    // Anchors
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: amountText.right;
                    anchors.right: parent.right;
                    height: 50;
                    // Style
                    leftPlaceholderGlyph: hifi.glyphs.hfc;
                    activeFocusOnPress: true;
                    activeFocusOnTab: true;

                    validator: IntValidator { bottom: 0; }

                    onAccepted: {
                        optionalMessage.focus = true;
                    }
                }

                RalewaySemiBold {
                    id: amountTextFieldError;
                    // Anchors
                    anchors.top: amountTextField.bottom;
                    anchors.topMargin: 2;
                    anchors.left: amountTextField.left;
                    anchors.right: amountTextField.right;
                    height: 40;
                    // Text size
                    size: 16;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignTop;
                    horizontalAlignment: Text.AlignRight;
                }
            }

            Item {
                id: messageContainer;
                anchors.top: amountContainer.bottom;
                anchors.topMargin: 16;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 140;
                
                FontLoader { id: firaSansSemiBold; source: "../../../../../fonts/FiraSans-SemiBold.ttf"; }
                TextArea {
                    id: optionalMessage;
                    property int maximumLength: 70;
                    property string previousText: text;
                    placeholderText: "Optional Message";
                    font.family: firaSansSemiBold.name;
                    font.pixelSize: 20;
                    // Anchors
                    anchors.fill: parent;
                    // Style
                    background: Rectangle {
                        anchors.fill: parent;
                        color: optionalMessage.activeFocus ? hifi.colors.white : hifi.colors.textFieldLightBackground;
                        border.width: optionalMessage.activeFocus ? 1 : 0;
                        border.color: optionalMessage.activeFocus ? hifi.colors.primaryHighlight : hifi.colors.textFieldLightBackground;
                    }
                    color: hifi.colors.black;
                    textFormat: TextEdit.PlainText;
                    wrapMode: TextEdit.Wrap;
                    activeFocusOnPress: true;
                    activeFocusOnTab: true;
                    // Workaround for no max length on TextAreas
                    onTextChanged: {
                        if (text.length > maximumLength) {
                            var cursor = cursorPosition;
                            text = previousText;
                            if (cursor > text.length) {
                                cursorPosition = text.length;
                            } else {
                                cursorPosition = cursor-1;
                            }
                        }
                        previousText = text;
                    }
                }
                RalewaySemiBold {
                    id: optionalMessageCharacterCount;
                    text: optionalMessage.text.length + "/" + optionalMessage.maximumLength;
                    // Anchors
                    anchors.top: optionalMessage.bottom;
                    anchors.topMargin: 2;
                    anchors.right: optionalMessage.right;
                    height: paintedHeight;
                    // Text size
                    size: 16;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignTop;
                    horizontalAlignment: Text.AlignRight;
                }
            }

            Item {
                id: bottomBarContainer;
                anchors.top: messageContainer.bottom;
                anchors.topMargin: 30;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;

                HifiControlsUit.CheckBox {
                    id: sendPubliclyCheckbox;
                    visible: false; // FIXME ONCE PARTICLE EFFECTS ARE IN
                    text: "Send Publicly"
                    // Anchors
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: parent.left;
                    anchors.right: cancelButton_sendMoneyStep.left;
                    anchors.rightMargin: 16;
                    boxSize: 24;
                }

                // "CANCEL" button
                HifiControlsUit.Button {
                    id: cancelButton_sendMoneyStep;
                    color: hifi.buttons.noneBorderless;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.right: sendButton.left;
                    anchors.rightMargin: 16;
                    anchors.verticalCenter: parent.verticalCenter;
                    height: 35;
                    width: 100;
                    text: "CANCEL";
                    onClicked: {
                        resetSendMoneyData();
                        root.nextActiveView = "sendMoneyHome";
                    }
                }

                // "SEND" button
                HifiControlsUit.Button {
                    id: sendButton;
                    color: hifi.buttons.blue;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.right: parent.right;
                    anchors.verticalCenter: parent.verticalCenter;
                    height: 35;
                    width: 100;
                    text: "SEND";
                    onClicked: {
                        if (parseInt(amountTextField.text) > parseInt(balanceText.text)) {
                            amountTextField.focus = true;
                            amountTextField.error = true;
                            amountTextFieldError.text = "<i>amount exceeds available funds</i>";
                        } else if (amountTextField.text === "" || parseInt(amountTextField.text) < 1) {
                            amountTextField.focus = true;
                            amountTextField.error = true;
                            amountTextFieldError.text = "<i>invalid amount</i>";
                        } else {
                            amountTextFieldError.text = "";
                            amountTextField.error = false;
                            root.isCurrentlySendingMoney = true;
                            amountTextField.focus = false;
                            optionalMessage.focus = false;
                            if (sendMoneyStep.referrer === "connections") {
                                Commerce.transferHfcToUsername(sendMoneyStep.selectedRecipientUserName, parseInt(amountTextField.text), optionalMessage.text);
                            } else if (sendMoneyStep.referrer === "nearby") {
                                Commerce.transferHfcToNode(sendMoneyStep.selectedRecipientNodeID, parseInt(amountTextField.text), optionalMessage.text);
                            }
                        }
                    }
                }
            }
        }
    }
    // Send Money Screen END
    
    // Sending Money Overlay START
    Rectangle {
        id: sendingMoneyOverlay;
        z: 998;

        visible: root.isCurrentlySendingMoney;
        anchors.fill: parent;
        color: Qt.rgba(0.0, 0.0, 0.0, 0.5);

        // This object is always used in a popup or full-screen Wallet section.
        // This MouseArea is used to prevent a user from being
        //     able to click on a button/mouseArea underneath the popup/section.
        MouseArea {
            anchors.fill: parent;
            propagateComposedEvents: false;
        }
                
        AnimatedImage {
            id: sendingMoneyImage;
            source: "../../../../../icons/profilePicLoading.gif"
            width: 160;
            height: width;
            anchors.top: parent.top;
            anchors.topMargin: 185;
            anchors.horizontalCenter: parent.horizontalCenter;
        }

        RalewaySemiBold {
            text: "Sending";
            // Anchors
            anchors.top: sendingMoneyImage.bottom;
            anchors.topMargin: 22;
            anchors.horizontalCenter: parent.horizontalCenter;
            width: paintedWidth;
            // Text size
            size: 24;
            // Style
            color: hifi.colors.white;
            verticalAlignment: Text.AlignVCenter;
        }
    }
    // Sending Money Overlay END
    
    // Payment Success BEGIN
    Rectangle {
        id: paymentSuccess;

        visible: root.currentActiveView === "paymentSuccess";
        anchors.fill: parent;
        color: "#AAAAAA";

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;

            RalewaySemiBold {
                id: paymentSentText;
                text: "Payment Sent";
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
                id: closeGlyphButton_paymentSuccess;
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
                        root.nextActiveView = "sendMoneyHome";
                        resetSendMoneyData();
                    }
                }
            }

            Item {
                id: sendToContainer_paymentSuccess;
                anchors.top: paymentSentText.bottom;
                anchors.topMargin: 20;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;

                RalewaySemiBold {
                    id: sendToText_paymentSuccess;
                    text: "Sent To:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: parent.left;
                    anchors.bottom: parent.bottom;
                    width: 90;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignVCenter;
                }

                RecipientDisplay {
                    anchors.top: parent.top;
                    anchors.left: sendToText_paymentSuccess.right;
                    anchors.right: parent.right;
                    height: parent.height;

                    displayName: sendMoneyStep.selectedRecipientDisplayName;
                    userName: sendMoneyStep.selectedRecipientUserName;
                    profilePic: sendMoneyStep.selectedRecipientProfilePic !== "" ? ((0 === sendMoneyStep.selectedRecipientProfilePic.indexOf("http")) ?
                        sendMoneyStep.selectedRecipientProfilePic : (Account.metaverseServerURL + sendMoneyStep.selectedRecipientProfilePic)) : "";
                    isDisplayingNearby: sendMoneyStep.referrer === "nearby";
                }
            }

            Item {
                id: amountContainer_paymentSuccess;
                anchors.top: sendToContainer_paymentSuccess.bottom;
                anchors.topMargin: 16;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;

                RalewaySemiBold {
                    id: amountText_paymentSuccess;
                    text: "Amount:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: parent.left;
                    anchors.bottom: parent.bottom;
                    width: 90;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignVCenter;
                }
                
                // "HFC" balance label
                HiFiGlyphs {
                    id: amountSentLabel;
                    text: hifi.glyphs.hfc;
                    // Size
                    size: 32;
                    // Anchors
                    anchors.left: amountText_paymentSuccess.right;
                    anchors.verticalCenter: parent.verticalCenter;
                    height: 50;
                    // Style
                    color: hifi.colors.baseGray;
                }

                RalewaySemiBold {
                    id: amountSentText;
                    text: amountTextField.text;
                    // Anchors
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: amountSentLabel.right;
                    anchors.leftMargin: 20;
                    anchors.right: parent.right;
                    height: 50;
                    // Style
                    size: 22;
                    color: hifi.colors.baseGray;
                }
            }

            RalewaySemiBold {
                id: optionalMessage_paymentSuccess;
                text: optionalMessage.text;
                // Anchors
                anchors.top: amountContainer_paymentSuccess.bottom;
                anchors.left: parent.left;
                anchors.leftMargin: 110;
                anchors.right: parent.right;
                anchors.rightMargin: 16;
                anchors.bottom: closeButton.top;
                anchors.bottomMargin: 40;
                // Text size
                size: 22;
                // Style
                color: hifi.colors.baseGray;
                wrapMode: Text.Wrap;
                verticalAlignment: Text.AlignTop;
            }

            // "Close" button
            HifiControlsUit.Button {
                id: closeButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 80;
                height: 50;
                width: 120;
                text: "Close";
                onClicked: {
                    root.nextActiveView = "sendMoneyHome";
                    resetSendMoneyData();
                }
            }
        }
    }
    // Payment Success END
    
    // Payment Failure BEGIN
    Rectangle {
        id: paymentFailure;

        visible: root.currentActiveView === "paymentFailure";
        anchors.fill: parent;
        color: "#AAAAAA";

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;

            RalewaySemiBold {
                id: paymentFailureText;
                text: "Payment Failed";
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
                id: closeGlyphButton_paymentFailure;
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
                        root.nextActiveView = "sendMoneyHome";
                        resetSendMoneyData();
                    }
                }
            }

            RalewaySemiBold {
                id: paymentFailureDetailText;
                text: "The recipient you specified was unable to receive your payment.";
                anchors.top: paymentFailureText.bottom;
                anchors.topMargin: 20;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;
                // Text size
                size: 18;
                // Style
                color: hifi.colors.baseGray;
                verticalAlignment: Text.AlignVCenter;
                wrapMode: Text.Wrap;
            }

            Item {
                id: sendToContainer_paymentFailure;
                anchors.top: paymentFailureDetailText.bottom;
                anchors.topMargin: 8;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;

                RalewaySemiBold {
                    id: sentToText_paymentFailure;
                    text: "Sent To:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: parent.left;
                    anchors.bottom: parent.bottom;
                    width: 90;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignVCenter;
                }

                RecipientDisplay {
                    anchors.top: parent.top;
                    anchors.left: sentToText_paymentFailure.right;
                    anchors.right: parent.right;
                    height: parent.height;

                    displayName: sendMoneyStep.selectedRecipientDisplayName;
                    userName: sendMoneyStep.selectedRecipientUserName;
                    profilePic: sendMoneyStep.selectedRecipientProfilePic !== "" ? ((0 === sendMoneyStep.selectedRecipientProfilePic.indexOf("http")) ?
                        sendMoneyStep.selectedRecipientProfilePic : (Account.metaverseServerURL + sendMoneyStep.selectedRecipientProfilePic)) : "";
                    isDisplayingNearby: sendMoneyStep.referrer === "nearby";
                }
            }

            Item {
                id: amountContainer_paymentFailure;
                anchors.top: sendToContainer_paymentFailure.bottom;
                anchors.topMargin: 16;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 80;

                RalewaySemiBold {
                    id: amountText_paymentFailure;
                    text: "Amount:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: parent.left;
                    anchors.bottom: parent.bottom;
                    width: 90;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignVCenter;
                }
                
                // "HFC" balance label
                HiFiGlyphs {
                    id: amountSentLabel_paymentFailure;
                    text: hifi.glyphs.hfc;
                    // Size
                    size: 32;
                    // Anchors
                    anchors.left: amountText_paymentFailure.right;
                    anchors.verticalCenter: parent.verticalCenter;
                    height: 50;
                    // Style
                    color: hifi.colors.baseGray;
                }

                RalewaySemiBold {
                    id: amountSentText_paymentFailure;
                    text: amountTextField.text;
                    // Anchors
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: amountSentLabel_paymentFailure.right;
                    anchors.leftMargin: 20;
                    anchors.right: parent.right;
                    height: 50;
                    // Style
                    size: 22;
                    color: hifi.colors.baseGray;
                }
            }

            RalewaySemiBold {
                id: optionalMessage_paymentFailuire;
                text: optionalMessage.text;
                // Anchors
                anchors.top: amountContainer_paymentFailure.bottom;
                anchors.left: parent.left;
                anchors.leftMargin: 110;
                anchors.right: parent.right;
                anchors.rightMargin: 16;
                anchors.bottom: closeButton_paymentFailure.top;
                anchors.bottomMargin: 40;
                // Text size
                size: 22;
                // Style
                color: hifi.colors.baseGray;
                wrapMode: Text.Wrap;
                verticalAlignment: Text.AlignTop;
            }

            // "Close" button
            HifiControlsUit.Button {
                id: closeButton_paymentFailure;
                color: hifi.buttons.noneBorderless;
                colorScheme: hifi.colorSchemes.dark;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 80;
                height: 50;
                width: 120;
                text: "Cancel";
                onClicked: {
                    root.nextActiveView = "sendMoneyHome";
                    resetSendMoneyData();
                }
            }

            // "Retry" button
            HifiControlsUit.Button {
                id: retryButton_paymentFailure;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: parent.right;
                anchors.rightMargin: 12;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 80;
                height: 50;
                width: 120;
                text: "Retry";
                onClicked: {
                    root.isCurrentlySendingMoney = true;
                    if (sendMoneyStep.referrer === "connections") {
                        Commerce.transferHfcToUsername(sendMoneyStep.selectedRecipientUserName, parseInt(amountTextField.text), optionalMessage.text);
                    } else if (sendMoneyStep.referrer === "nearby") {
                        Commerce.transferHfcToNode(sendMoneyStep.selectedRecipientNodeID, parseInt(amountTextField.text), optionalMessage.text);
                    }
                }
            }
        }
    }
    // Payment Failure END


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

    function resetSendMoneyData() {
        amountTextField.focus = false;
        optionalMessage.focus = false;
        amountTextFieldError.text = "";
        amountTextField.error = false;
        chooseRecipientNearby.selectedRecipient = "";
        sendMoneyStep.selectedRecipientNodeID = "";
        sendMoneyStep.selectedRecipientDisplayName = "";
        sendMoneyStep.selectedRecipientUserName = "";
        sendMoneyStep.selectedRecipientProfilePic = "";
        amountTextField.text = "";
        optionalMessage.text = "";
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
            case 'selectRecipient':
                if (message.isSelected) {
                    chooseRecipientNearby.selectedRecipient = message.id[0];
                    sendMoneyStep.selectedRecipientDisplayName = message.displayName;
                    sendMoneyStep.selectedRecipientUserName = message.userName;
                } else {
                    chooseRecipientNearby.selectedRecipient = "";
                    sendMoneyStep.selectedRecipientDisplayName = '';
                    sendMoneyStep.selectedRecipientUserName = '';
                }
            break;
            case 'updateSelectedRecipientUsername':
                sendMoneyStep.selectedRecipientUserName = message.userName;
            break;
            default:
                console.log('SendMoney: Unrecognized message from wallet.js:', JSON.stringify(message));
        }
    }
    signal sendSignalToWallet(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
