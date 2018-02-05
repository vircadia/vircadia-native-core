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
    property bool shouldShowTopAndBottomOfWallet: chooseRecipientConnection.visible ||
        chooseRecipientNearby.visible || paymentSuccess.visible || paymentFailure.visible;
    property bool shouldShowTopOfWallet: sendMoneyStep.visible;
    property bool isCurrentlySendingMoney: false;
        
    // This object is always used in a popup or full-screen Wallet section.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup/section.
    MouseArea {
        x: 0;
        y: (root.shouldShowTopAndBottomOfWallet && !root.shouldShowTopOfWallet) ? 0 : root.parentAppTitleBarHeight;
        width: parent.width;
        height: (root.shouldShowTopAndBottomOfWallet || root.shouldShowTopOfWallet) ? parent.height : parent.height - root.parentAppTitleBarHeight - root.parentAppNavBarHeight;
        propagateComposedEvents: false;
        hoverEnabled: true;
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
                if (sendPubliclyCheckbox.checked && sendMoneyStep.referrer === "nearby") {
                    sendSignalToWallet({method: 'sendMoney_sendPublicly', recipient: sendMoneyStep.selectedRecipientNodeID, amount: parseInt(amountTextField.text)});
                }
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

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;
    }

    // Send Money Home BEGIN
    Item {
        id: sendMoneyHome;
        z: 996;
        visible: root.currentActiveView === "sendMoneyHome" || root.currentActiveView === "chooseRecipientConnection" || root.currentActiveView === "chooseRecipientNearby";
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
        
    
            LinearGradient {
                anchors.fill: parent;
                start: Qt.point(0, 0);
                end: Qt.point(0, height);
                gradient: Gradient {
                    GradientStop { position: 0.0; color: hifi.colors.white }
                    GradientStop { position: 1.0; color: hifi.colors.faintGray }
                }
            }

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
                    source: "./images/connection.svg";
                    height: 70;
                    width: parent.width;
                    fillMode: Image.PreserveAspectFit;
                    horizontalAlignment: Image.AlignHCenter;
                    verticalAlignment: Image.AlignTop;
                    mipmap: true;
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
                        filterBar.text = "";
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
                    source: "./images/nearby.svg";
                    height: 70;
                    width: parent.width;
                    fillMode: Image.PreserveAspectFit;
                    horizontalAlignment: Image.AlignHCenter;
                    verticalAlignment: Image.AlignTop;
                    mipmap: true;
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
        z: 997;
        visible: root.currentActiveView === "chooseRecipientConnection";
        anchors.fill: parent;
        color: "#80000000";

        // This object is always used in a popup or full-screen Wallet section.
        // This MouseArea is used to prevent a user from being
        //     able to click on a button/mouseArea underneath the popup/section.
        MouseArea {
            anchors.fill: parent;
            propagateComposedEvents: false;
            hoverEnabled: true;
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
            color: "#FFFFFF";
            radius: 8;

            RalewaySemiBold {
                id: chooseRecipientText_connections;
                text: "Choose Recipient:";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 26;
                anchors.left: parent.left;
                anchors.leftMargin: 36;
                width: paintedWidth;
                height: 30;
                // Text size
                size: 18;
                // Style
                color: hifi.colors.baseGray;
            }
            
            HiFiGlyphs {
                id: closeGlyphButton_connections;
                text: hifi.glyphs.close;
                color: hifi.colors.lightGrayText;
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
                anchors.leftMargin: 36;
                anchors.right: parent.right;
                anchors.rightMargin: 16;
                anchors.top: chooseRecipientText_connections.bottom;
                anchors.topMargin: 12;

                HifiControlsUit.TextField {
                    id: filterBar;
                    colorScheme: hifi.colorSchemes.faintGray;
                    hasClearButton: true;
                    hasRoundedBorder: true;
                    hasDefocusedBorder: false;
                    roundedBorderRadius: filterBar.height/2;
                    anchors.fill: parent;
                    centerPlaceholderGlyph: hifi.glyphs.search;

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
                            hoverEnabled: true;
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
        z: 997;
        color: "#80000000";

        property string selectedRecipient;

        visible: root.currentActiveView === "chooseRecipientNearby";
        anchors.fill: parent;

        // This object is always used in a popup or full-screen Wallet section.
        // This MouseArea is used to prevent a user from being
        //     able to click on a button/mouseArea underneath the popup/section.
        MouseArea {
            anchors.fill: parent;
            propagateComposedEvents: false;
            hoverEnabled: true;
        }

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;
            color: "#FFFFFF";
            radius: 8;

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
                size: 18;
                // Style
                color: hifi.colors.baseGray;
            }
            
            HiFiGlyphs {
                id: closeGlyphButton_nearby;
                text: hifi.glyphs.close;
                color: hifi.colors.lightGrayText;
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
                id: selectionInstructions;
                text: chooseRecipientNearby.selectedRecipient === "" ? "Trigger or click on\nsomeone nearby to select them" : 
                    "Trigger or click on\nsomeone else to select again";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 100;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
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

            Image {
                anchors.top: selectionInstructions.bottom;
                anchors.topMargin: 20;
                anchors.bottom: selectionMadeContainer.top;
                anchors.bottomMargin: 30;
                source: "./images/p2p-nearby-unselected.svg";
                width: parent.width;
                fillMode: Image.PreserveAspectFit;
                horizontalAlignment: Image.AlignHCenter;
                verticalAlignment: Image.AlignVCenter;
                mipmap: true;
            }

            Rectangle {
                id: selectionMadeContainer;
                visible: chooseRecipientNearby.selectedRecipient !== "";
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 190;
                color: "#F3F3F3";
                radius: 8;

                // Used to square off the top left and top right edges of this container
                Rectangle {
                    color: "#F3F3F3";
                    height: selectionMadeContainer.radius;
                    anchors.top: selectionMadeContainer.top;
                    anchors.left: selectionMadeContainer.left;
                    anchors.right: selectionMadeContainer.right;
                }

                RalewaySemiBold {
                    id: sendToText;
                    text: "Send to:";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.topMargin: 36;
                    anchors.left: parent.left;
                    anchors.leftMargin: 36;
                    width: paintedWidth;
                    height: paintedHeight;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                }

                Image {
                    id: selectedImage;
                    anchors.top: parent.top;
                    anchors.topMargin: 24;
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: 32;
                    anchors.left: sendToText.right;
                    anchors.leftMargin: 4;
                    source: "./images/p2p-nearby-selected.svg";
                    width: 50;
                    fillMode: Image.PreserveAspectFit;
                    horizontalAlignment: Image.AlignHCenter;
                    verticalAlignment: Image.AlignVCenter;
                    mipmap: true;
                }

                RalewaySemiBold {
                    id: avatarDisplayName;
                    text: '"' + AvatarList.getAvatar(chooseRecipientNearby.selectedRecipient).sessionDisplayName + '"';
                    // Anchors
                    anchors.top: parent.top;
                    anchors.topMargin: 34;
                    anchors.left: selectedImage.right;
                    anchors.leftMargin: 10;
                    anchors.right: parent.right;
                    anchors.rightMargin: 10;
                    height: paintedHeight;
                    // Text size
                    size: 20;
                    // Style
                    color: hifi.colors.blueAccent;
                }

                RalewaySemiBold {
                    id: avatarUserName;
                    text: sendMoneyStep.selectedRecipientUserName;
                    // Anchors
                    anchors.top: avatarDisplayName.bottom;
                    anchors.topMargin: 16;
                    anchors.left: selectedImage.right;
                    anchors.leftMargin: 10;
                    anchors.right: parent.right;
                    anchors.rightMargin: 10;
                    height: paintedHeight;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                }

                // "CHOOSE" button
                HifiControlsUit.Button {
                    id: chooseButton;
                    color: hifi.buttons.blue;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.horizontalCenter: parent.horizontalCenter;
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: 20;
                    height: 40;
                    width: 110;
                    text: "CHOOSE";
                    onClicked: {
                        sendMoneyStep.referrer = "nearby";
                        sendMoneyStep.selectedRecipientNodeID = chooseRecipientNearby.selectedRecipient;
                        chooseRecipientNearby.selectedRecipient = "";

                        root.nextActiveView = "sendMoneyStep";
                    }
                }
            }
        }
    }
    // Choose Recipient Nearby END
    
    // Send Money Screen BEGIN
    Item {
        id: sendMoneyStep;
        z: 996;

        property string referrer; // either "connections" or "nearby"
        property string selectedRecipientNodeID;
        property string selectedRecipientDisplayName;
        property string selectedRecipientUserName;
        property string selectedRecipientProfilePic;

        visible: root.currentActiveView === "sendMoneyStep";
        anchors.fill: parent;
        anchors.topMargin: root.parentAppTitleBarHeight;

        RalewaySemiBold {
            id: sendMoneyText_sendMoneyStep;
            text: "Send Money";
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
            color: hifi.colors.white;
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
                text: "Send to:";
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.bottom: parent.bottom;
                width: 90;
                // Text size
                size: 18;
                // Style
                color: hifi.colors.white;
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
                color: hifi.buttons.none;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: parent.right;
                anchors.verticalCenter: parent.verticalCenter;
                height: 35;
                width: 100;
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
                color: hifi.colors.white;
                verticalAlignment: Text.AlignVCenter;
            }

            HifiControlsUit.TextField {
                id: amountTextField;
                colorScheme: hifi.colorSchemes.dark;
                inputMethodHints: Qt.ImhDigitsOnly;
                // Anchors
                anchors.verticalCenter: parent.verticalCenter;
                anchors.left: amountText.right;
                anchors.right: parent.right;
                height: 50;
                // Style
                leftPermanentGlyph: hifi.glyphs.hfc;
                activeFocusOnPress: true;
                activeFocusOnTab: true;

                validator: IntValidator { bottom: 0; }

                onAccepted: {
                    optionalMessage.focus = true;
                }
            }

            FiraSansSemiBold {
                visible: amountTextFieldError.text === "";
                text: "Balance: ";
                // Anchors
                anchors.top: amountTextField.bottom;
                anchors.topMargin: 2;
                anchors.left: amountTextField.left;
                anchors.right: sendMoneyBalanceText_HFC.left;
                width: paintedWidth;
                height: 40;
                // Text size
                size: 16;
                // Style
                color: hifi.colors.lightGrayText;
                verticalAlignment: Text.AlignTop;
                horizontalAlignment: Text.AlignRight;
            }
            HiFiGlyphs {
                id: sendMoneyBalanceText_HFC;
                visible: amountTextFieldError.text === "";
                text: hifi.glyphs.hfc;
                // Size
                size: 16;
                // Anchors
                anchors.top: amountTextField.bottom;
                anchors.topMargin: 2;
                anchors.right: sendMoneyBalanceText.left;
                width: paintedWidth;
                height: 40;
                // Style
                color: hifi.colors.lightGrayText;
                verticalAlignment: Text.AlignTop;
                horizontalAlignment: Text.AlignRight;
            }
            FiraSansSemiBold {
                id: sendMoneyBalanceText;
                visible: amountTextFieldError.text === "";
                text: balanceText.text;
                // Anchors
                anchors.top: amountTextField.bottom;
                anchors.topMargin: 2;
                anchors.right: amountTextField.right;
                width: paintedWidth;
                height: 40;
                // Text size
                size: 16;
                // Style
                color: hifi.colors.lightGrayText;
                verticalAlignment: Text.AlignTop;
                horizontalAlignment: Text.AlignRight;
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
                color: hifi.colors.white;
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
                property int maximumLength: 72;
                property string previousText: text;
                placeholderText: "<i>Optional Public Message (" + maximumLength + " character limit)</i>";
                font.family: firaSansSemiBold.name;
                font.pixelSize: 20;
                // Anchors
                anchors.fill: parent;
                // Style
                background: Rectangle {
                    anchors.fill: parent;
                    color: optionalMessage.activeFocus ? hifi.colors.black : hifi.colors.baseGrayShadow;
                    border.width: optionalMessage.activeFocus ? 1 : 0;
                    border.color: optionalMessage.activeFocus ? hifi.colors.primaryHighlight : hifi.colors.textFieldLightBackground;
                }
                color: hifi.colors.white;
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
            FiraSansSemiBold {
                id: optionalMessageCharacterCount;
                text: optionalMessage.text.length + "/" + optionalMessage.maximumLength;
                // Anchors
                anchors.top: optionalMessage.bottom;
                anchors.topMargin: 4;
                anchors.right: optionalMessage.right;
                height: paintedHeight;
                // Text size
                size: 16;
                // Style
                color: optionalMessage.text.length === optionalMessage.maximumLength ? "#ea89a5" : hifi.colors.lightGrayText;
                verticalAlignment: Text.AlignTop;
                horizontalAlignment: Text.AlignRight;
            }
        }

        HifiControlsUit.CheckBox {
            id: sendPubliclyCheckbox;
            visible: sendMoneyStep.referrer === "nearby";
            checked: Settings.getValue("sendMoneyNearbyPublicly", true);
            text: "Show Effect"
            // Anchors
            anchors.top: messageContainer.bottom;
            anchors.topMargin: 16;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: 110;
            boxSize: 28;
            onCheckedChanged: {
                Settings.setValue("sendMoneyNearbyPublicly", checked);
            }
        }
        RalewaySemiBold {
            id: sendPubliclyCheckboxHelp;
            visible: sendPubliclyCheckbox.visible;
            text: "[?]";
            // Anchors
            anchors.left: sendPubliclyCheckbox.right;
            anchors.leftMargin: 8;
            anchors.verticalCenter: sendPubliclyCheckbox.verticalCenter;
            height: 30;
            width: paintedWidth;
            // Text size
            size: 18;
            // Style
            color: hifi.colors.blueAccent;
            MouseArea {
                enabled: sendPubliclyCheckboxHelp.visible;
                anchors.fill: parent;
                hoverEnabled: true;
                onEntered: {
                    parent.color = hifi.colors.blueHighlight;
                }
                onExited: {
                    parent.color = hifi.colors.blueAccent;
                }
                onClicked: {
                    lightboxPopup.titleText = "Send Money Effect";
                    lightboxPopup.bodyImageSource = "../wallet/sendMoney/images/send-money-effect-sm.jpg"; // Path relative to CommerceLightbox.qml
                    lightboxPopup.bodyText = "Enabling this option will create a particle effect between you and " +
                        "your recipient that is visible to everyone nearby.";
                    lightboxPopup.button1text = "CLOSE";
                    lightboxPopup.button1method = "root.visible = false;"
                    lightboxPopup.visible = true;
                }
            }
        }

        Item {
            id: bottomBarContainer;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 20;
            height: 60;

            // "CANCEL" button
            HifiControlsUit.Button {
                id: cancelButton_sendMoneyStep;
                color: hifi.buttons.noneBorderlessWhite;
                colorScheme: hifi.colorSchemes.dark;
                anchors.left: parent.left;
                anchors.leftMargin: 24;
                anchors.verticalCenter: parent.verticalCenter;
                height: 40;
                width: 150;
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
                anchors.rightMargin: 24;
                anchors.verticalCenter: parent.verticalCenter;
                height: 40;
                width: 150;
                text: "SUBMIT";
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
    // Send Money Screen END
    
    // Sending Money Overlay START
    Rectangle {
        id: sendingMoneyOverlay;
        z: 998;

        visible: root.isCurrentlySendingMoney;
        anchors.fill: parent;
        color: Qt.rgba(0.0, 0.0, 0.0, 0.8);

        // This object is always used in a popup or full-screen Wallet section.
        // This MouseArea is used to prevent a user from being
        //     able to click on a button/mouseArea underneath the popup/section.
        MouseArea {
            anchors.fill: parent;
            propagateComposedEvents: false;
            hoverEnabled: true;
        }
                
        AnimatedImage {
            id: sendingMoneyImage;
            source: "./images/loader.gif"
            width: 96;
            height: width;
            anchors.verticalCenter: parent.verticalCenter;
            anchors.horizontalCenter: parent.horizontalCenter;
        }

        RalewaySemiBold {
            text: "Sending";
            // Anchors
            anchors.top: sendingMoneyImage.bottom;
            anchors.topMargin: 4;
            anchors.horizontalCenter: parent.horizontalCenter;
            width: paintedWidth;
            // Text size
            size: 26;
            // Style
            color: hifi.colors.white;
            verticalAlignment: Text.AlignVCenter;
        }
    }
    // Sending Money Overlay END
    
    // Payment Success BEGIN
    Item {
        id: paymentSuccess;

        visible: root.currentActiveView === "paymentSuccess";
        anchors.fill: parent;

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;
            color: "#FFFFFF";

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
                color: hifi.colors.lightGrayText;
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
                    textColor: hifi.colors.blueAccent;

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
                    color: hifi.colors.blueAccent;
                }

                FiraSansSemiBold {
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
                    color: hifi.colors.blueAccent;
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
                color: hifi.colors.blueAccent;
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
    Item {
        id: paymentFailure;

        visible: root.currentActiveView === "paymentFailure";
        anchors.fill: parent;

        Rectangle {
            anchors.centerIn: parent;
            width: parent.width - 30;
            height: parent.height - 30;
            color: "#FFFFFF";

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
                color: hifi.colors.lightGrayText;
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
                    textColor: hifi.colors.baseGray;

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

                FiraSansSemiBold {
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
        sendPubliclyCheckbox.checked = Settings.getValue("sendMoneyNearbyPublicly", true);
        sendMoneyStep.referrer = "";
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
