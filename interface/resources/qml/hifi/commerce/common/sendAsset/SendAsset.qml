//
//  SendAsset.qml
//  qml/hifi/commerce/common/sendAsset
//
//  SendAsset
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
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../../controls" as HifiControls
import "../" as HifiCommerceCommon
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.

Item {
    HifiConstants { id: hifi; }

    id: root;

    property int parentAppTitleBarHeight;
    property int parentAppNavBarHeight;
    property string currentActiveView: "sendAssetHome";
    property string nextActiveView: "";
    property bool shouldShowTopAndBottomOfParent: chooseRecipientConnection.visible ||
        chooseRecipientNearby.visible || paymentSuccess.visible || paymentFailure.visible;
    property bool shouldShowTopOfParent: sendAssetStep.visible;
    property bool isCurrentlySendingAsset: false;
    property string assetName: "";
    property string assetCertID: "";
    property string sendingPubliclyEffectImage;
    property var http;
    property var listModelName;
    property var keyboardContainer: nil;
        
    // This object is always used in a popup or full-screen Wallet section.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup/section.
    MouseArea {
        x: 0;
        y: (root.shouldShowTopAndBottomOfParent && !root.shouldShowTopOfParent) ? 0 : root.parentAppTitleBarHeight;
        width: parent.width;
        height: (root.shouldShowTopAndBottomOfParent || root.shouldShowTopOfParent) ? parent.height : parent.height - root.parentAppTitleBarHeight - root.parentAppNavBarHeight;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    // Background
    Rectangle {
        z: 1;
        visible: root.assetName !== "" && sendAssetStep.visible;
        anchors.top: parent.top;
        anchors.topMargin: root.parentAppTitleBarHeight;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;
        color: hifi.colors.white;
    }

    Connections {
        target: Commerce;

        onBalanceResult : {
            balanceText.text = result.data.balance;
        }

        onTransferAssetToNodeResult: {
            if (!root.visible) {
                return;
            }

            root.isCurrentlySendingAsset = false;

            if (result.status === 'success') {
                root.nextActiveView = 'paymentSuccess';
                if (sendPubliclyCheckbox.checked && sendAssetStep.referrer === "nearby") {
                    sendSignalToParent({
                        method: 'sendAsset_sendPublicly',
                        assetName: root.assetName,
                        recipient: sendAssetStep.selectedRecipientNodeID,
                        amount: parseInt(amountTextField.text),
                        effectImage: root.sendingPubliclyEffectImage
                    });
                }
            } else {
                root.nextActiveView = 'paymentFailure';
            }
        }

        onTransferAssetToUsernameResult: {
            if (!root.visible) {
                return;
            }

            root.isCurrentlySendingAsset = false;

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
            sendSignalToParent({method: 'disable_ChooseRecipientNearbyMode'});
        }

        root.currentActiveView = root.nextActiveView;

        if (root.currentActiveView === 'chooseRecipientConnection') {
            // Refresh connections model
            connectionsModel.getFirstPage();
        } else if (root.currentActiveView === 'sendAssetHome') {
            Commerce.balance();
        } else if (root.currentActiveView === 'chooseRecipientNearby') {
            sendSignalToParent({method: 'enable_ChooseRecipientNearbyMode'});
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;
    }

    // Send Asset Home BEGIN
    Item {
        id: sendAssetHome;
        z: 996;
        visible: root.currentActiveView === "sendAssetHome" || root.currentActiveView === "chooseRecipientConnection" || root.currentActiveView === "chooseRecipientNearby";
        anchors.fill: parent;
        anchors.topMargin: root.parentAppTitleBarHeight;
        anchors.bottomMargin: root.parentAppNavBarHeight;

        Item {
            id: userInfoContainer;
            visible: root.assetName === "";
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 160;

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
        }

        // Send Asset
        Rectangle {
            id: sendAssetContainer;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
            anchors.top: userInfoContainer.visible ? undefined : parent.top;
            height: userInfoContainer.visible ? 440 : undefined;
            color: hifi.colors.white;
    
            LinearGradient {
                anchors.fill: parent;
                visible: root.assetName === "";
                start: Qt.point(0, 0);
                end: Qt.point(0, height);
                gradient: Gradient {
                    GradientStop { position: 0.0; color: hifi.colors.white }
                    GradientStop { position: 1.0; color: hifi.colors.faintGray }
                }
            }

            RalewaySemiBold {
                id: sendAssetText;
                text: root.assetName === "" ? "Send Money To:" : "Gift \"" + root.assetName + "\" To:";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 26;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                elide: Text.ElideRight;
                height: 30;
                // Text size
                size: 22;
                // Style
                color: hifi.colors.baseGray;
            }

            Item {
                id: connectionButton;
                // Anchors
                anchors.top: sendAssetText.bottom;
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
                anchors.top: sendAssetText.bottom;
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

            HifiControlsUit.Button {
                id: backButton_sendAssetHome;
                visible: parentAppNavBarHeight === 0;
                color: hifi.buttons.white;
                colorScheme: hifi.colorSchemes.light;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 40;
                height: 40;
                text: "BACK";
                onClicked: {
                    resetSendAssetData();
                    sendSignalToParent({method: 'sendAssetHome_back'});
                }
            }
        }
    }
    // Send Asset Home END
    
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
        
        HifiModels.PSFListModel {
            id: connectionsModel;
            http: root.http;
            listModelName: root.listModelName;
            endpoint: "/api/v1/users?filter=connections";
            itemsPerPage: 9;
            listView: connectionsList;
            processPage: function (data) {
                return data.users;
            };
            searchFilter: filterBar.text;
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
                color: root.assetName === "" ? hifi.colors.lightGrayText : hifi.colors.baseGray;
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
                        root.nextActiveView = "sendAssetHome";
                    }
                }
            }

            //
            // FILTER BAR START
            //
            Item {
                id: filterBarContainer;
                visible: !connectionInstructions.visible;
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
                    visible: !connectionsModel.retrievedAtLeastOnePage;
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
                    model: connectionsModel;
                    snapMode: ListView.SnapToItem;
                    // Anchors
                    anchors.fill: parent;
                    delegate: ConnectionItem {
                        isSelected: connectionsList.currentIndex === index;
                        userName: model.username;
                        profilePicUrl: model.images.thumbnail;
                        anchors.topMargin: 6;
                        anchors.bottomMargin: 6;

                        Connections {
                            onSendToParent: {
                                sendAssetStep.referrer = "connections";
                                sendAssetStep.selectedRecipientNodeID = '';
                                sendAssetStep.selectedRecipientDisplayName = 'connection';
                                sendAssetStep.selectedRecipientUserName = msg.userName;
                                sendAssetStep.selectedRecipientProfilePic = msg.profilePicUrl;

                                root.nextActiveView = "sendAssetStep";
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
                
                // "Make a Connection" instructions
                Rectangle {
                    id: connectionInstructions;
                    visible: connectionsModel.count === 0 && !connectionsModel.searchFilter && !connectionsLoading.visible;
                    anchors.fill: parent;
                    color: "white";

                    RalewayRegular {
                        id: makeAConnectionText;
                        // Properties
                        text: "Make a Connection";
                        // Anchors
                        anchors.top: parent.top;
                        anchors.topMargin: 20;
                        anchors.left: parent.left;
                        anchors.right: parent.right;
                        // Text Size
                        size: 24;
                        // Text Positioning
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter;
                        // Style
                        color: hifi.colors.darkGray;
                    }

                    Image {
                        id: connectionImage;
                        source: "qrc:/icons/connection.svg";
                        width: 150;
                        height: 150;
                        mipmap: true;
                        // Anchors
                        anchors.top: makeAConnectionText.bottom;
                        anchors.topMargin: 15;
                        anchors.horizontalCenter: parent.horizontalCenter;
                    }

                    FontLoader { id: ralewayRegular; source: "qrc:/fonts/Raleway-Regular.ttf"; }
                    Text {
                        id: connectionHelpText;
                        // Anchors
                        anchors.top: connectionImage.bottom;
                        anchors.topMargin: 15;
                        anchors.left: parent.left
                        anchors.leftMargin: 40;
                        anchors.right: parent.right
                        anchors.rightMargin: 40;
                        // Text alignment
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHLeft
                        // Style
                        font.pixelSize: 18;
                        font.family: ralewayRegular.name
                        color: hifi.colors.darkGray;
                        wrapMode: Text.Wrap
                        textFormat: Text.StyledText;
                        property string instructions:
                            "<b>When you meet someone you want to remember later, you can <font color='purple'>connect</font> with a handshake:</b><br><br>"
                        property string hmdMountedInstructions:
                            "1. Put your hand out onto their hand and squeeze your controller's grip button on its side.<br>" +
                            "2. Once the other person puts their hand onto yours, you'll see your connection form.<br>" +
                            "3. After about 3 seconds, you're connected!"
                        property string hmdNotMountedInstructions:
                            "1. Press and hold the 'x' key to extend your arm.<br>" +
                            "2. Once the other person puts their hand onto yours, you'll see your connection form.<br>" +
                            "3. After about 3 seconds, you're connected!";
                        // Text
                        text:
                            HMD.mounted ? instructions + hmdMountedInstructions : instructions + hmdNotMountedInstructions
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
                color: root.assetName === "" ? hifi.colors.lightGrayText : hifi.colors.baseGray;
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
                        root.nextActiveView = "sendAssetHome";
                        resetSendAssetData();
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
                    text: root.assetName === "" ? "Send to:" : "Gift to:";
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
                    text: sendAssetStep.selectedRecipientUserName;
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
                        sendAssetStep.referrer = "nearby";
                        sendAssetStep.selectedRecipientNodeID = chooseRecipientNearby.selectedRecipient;
                        chooseRecipientNearby.selectedRecipient = "";

                        root.nextActiveView = "sendAssetStep";
                    }
                }
            }
        }
    }
    // Choose Recipient Nearby END
    
    // Send Asset Screen BEGIN
    Item {
        id: sendAssetStep;
        z: 996;

        property string referrer; // either "connections" or "nearby"
        property string selectedRecipientNodeID;
        property string selectedRecipientDisplayName;
        property string selectedRecipientUserName;
        property string selectedRecipientProfilePic;

        visible: root.currentActiveView === "sendAssetStep" || paymentSuccess.visible || paymentFailure.visible;
        anchors.fill: parent;
        anchors.topMargin: root.parentAppTitleBarHeight;

        RalewaySemiBold {
            id: sendAssetText_sendAssetStep;
            text: root.assetName === "" ? "Send Money" : "Gift \"" + root.assetName + "\"";
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 26;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            elide: Text.ElideRight;
            height: 30;
            // Text size
            size: 22;
            // Style
            color: root.assetName === "" ? hifi.colors.white : hifi.colors.black;
        }

        Item {
            id: sendToContainer;
            anchors.top: sendAssetText_sendAssetStep.bottom;
            anchors.topMargin: 20;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            height: 80;

            RalewaySemiBold {
                id: sendToText_sendAssetStep;
                text: root.assetName === "" ? "Send to:" : "Gift to:";
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.bottom: parent.bottom;
                width: 90;
                // Text size
                size: 18;
                // Style
                color: root.assetName === "" ? hifi.colors.white : hifi.colors.black;
                verticalAlignment: Text.AlignVCenter;
            }

            RecipientDisplay {
                anchors.top: parent.top;
                anchors.left: sendToText_sendAssetStep.right;
                anchors.right: changeButton.left;
                anchors.rightMargin: 12;
                height: parent.height;
                textColor: root.assetName === "" ? hifi.colors.white : hifi.colors.black;

                displayName: sendAssetStep.selectedRecipientDisplayName;
                userName: sendAssetStep.selectedRecipientUserName;
                profilePic: sendAssetStep.selectedRecipientProfilePic !== "" ? ((0 === sendAssetStep.selectedRecipientProfilePic.indexOf("http")) ?
                    sendAssetStep.selectedRecipientProfilePic : (Account.metaverseServerURL + sendAssetStep.selectedRecipientProfilePic)) : "";
                isDisplayingNearby: sendAssetStep.referrer === "nearby";
            }

            // "CHANGE" button
            HifiControlsUit.Button {
                id: changeButton;
                color: root.assetName === "" ? hifi.buttons.none : hifi.buttons.white;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: parent.right;
                anchors.verticalCenter: parent.verticalCenter;
                height: 35;
                width: 100;
                text: "CHANGE";
                onClicked: {
                    if (sendAssetStep.referrer === "connections") {
                        root.nextActiveView = "chooseRecipientConnection";
                    } else if (sendAssetStep.referrer === "nearby") {
                        root.nextActiveView = "chooseRecipientNearby";
                    }
                    resetSendAssetData();
                }
            }
        }

        Item {
            id: amountContainer;
            visible: root.assetName === "";
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
                text: root.assetName === "" ? "" : "1";
                colorScheme: root.assetName === "" ? hifi.colorSchemes.dark : hifi.colorSchemes.light;
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
                anchors.right: sendAssetBalanceText_HFC.left;
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
                id: sendAssetBalanceText_HFC;
                visible: amountTextFieldError.text === "";
                text: hifi.glyphs.hfc;
                // Size
                size: 16;
                // Anchors
                anchors.top: amountTextField.bottom;
                anchors.topMargin: 2;
                anchors.right: sendAssetBalanceText.left;
                width: paintedWidth;
                height: 40;
                // Style
                color: hifi.colors.lightGrayText;
                verticalAlignment: Text.AlignTop;
                horizontalAlignment: Text.AlignRight;
            }
            FiraSansSemiBold {
                id: sendAssetBalanceText;
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
            anchors.top: amountContainer.visible ? amountContainer.bottom : sendToContainer.bottom;
            anchors.topMargin: 16;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            height: 95;

            TextArea {
                id: optionalMessage;
                property int maximumLength: 72;
                property string previousText: text;
                placeholderText: "<i>Optional Public Message (" + maximumLength + " character limit)</i>";
                font.family: "Fira Sans SemiBold";
                font.pixelSize: 20;
                // Anchors
                anchors.fill: parent;
                // Style
                background: Rectangle {
                    anchors.fill: parent;
                    color: root.assetName === "" ? (optionalMessage.activeFocus ? hifi.colors.black : hifi.colors.baseGrayShadow) :
                        (optionalMessage.activeFocus ? "#EFEFEF" : "#EEEEEE");
                    border.width: optionalMessage.activeFocus ? 1 : 0;
                    border.color: optionalMessage.activeFocus ? hifi.colors.primaryHighlight : hifi.colors.textFieldLightBackground;
                }
                color: root.assetName === "" ? hifi.colors.white : hifi.colors.black;
                textFormat: TextEdit.PlainText;
                wrapMode: TextEdit.Wrap;
                activeFocusOnPress: true;
                activeFocusOnTab: true;
                onTextChanged: {
                    // Don't allow tabs or newlines
                    if ((/[\n\r\t]/g).test(text)) {
                        var cursor = cursorPosition;
                        text = text.replace(/[\n\r\t]/g, '');
                        cursorPosition = cursor-1;
                    }
                    // Workaround for no max length on TextAreas
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
                color: optionalMessage.text.length === optionalMessage.maximumLength ? "#ea89a5" : (root.assetName === "" ? hifi.colors.lightGrayText : hifi.colors.baseGrayHighlight);
                verticalAlignment: Text.AlignTop;
                horizontalAlignment: Text.AlignRight;
            }
        }

        HifiControlsUit.CheckBox {
            id: sendPubliclyCheckbox;
            visible: sendAssetStep.referrer === "nearby";
            checked: Settings.getValue("sendAssetsNearbyPublicly", true);
            text: "Show Effect"
            // Anchors
            anchors.verticalCenter: bottomBarContainer.verticalCenter;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            width: 130;
            boxSize: 28;
            onCheckedChanged: {
                Settings.setValue("sendAssetsNearbyPublicly", checked);
            }
        }
        RalewaySemiBold {
            id: sendPubliclyCheckboxHelp;
            visible: sendPubliclyCheckbox.visible;
            text: "[?]";
            // Anchors
            anchors.left: sendPubliclyCheckbox.right;
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
                    lightboxPopup.titleText = (root.assetName === "" ? "Send Effect" : "Gift Effect");
                    lightboxPopup.bodyImageSource = "sendAsset/images/send-money-effect-sm.jpg"; // Path relative to CommerceLightbox.qml
                    lightboxPopup.bodyText = "Enabling this option will create a particle effect between you and " +
                        "your recipient that is visible to everyone nearby.";
                    lightboxPopup.button1text = "CLOSE";
                    lightboxPopup.button1method = function() {
                        lightboxPopup.visible = false;
                    }
                    lightboxPopup.visible = true;
                    if (keyboardContainer) {
                        keyboardContainer.keyboardRaised = false;
                    }
                }
            }
        }

        Item {
            id: bottomBarContainer;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            anchors.top: messageContainer.bottom;
            anchors.topMargin: 20;
            height: 60;

            // "CANCEL" button
            HifiControlsUit.Button {
                id: cancelButton_sendAssetStep;
                color: root.assetName === "" ? hifi.buttons.noneBorderlessWhite : hifi.buttons.noneBorderlessGray;
                colorScheme: hifi.colorSchemes.dark;
                anchors.right: sendButton.left;
                anchors.rightMargin: 24;
                anchors.verticalCenter: parent.verticalCenter;
                height: 40;
                width: 100;
                text: "CANCEL";
                onClicked: {
                    resetSendAssetData();
                    root.nextActiveView = "sendAssetHome";
                }
            }

            // "SEND" button
            HifiControlsUit.Button {
                id: sendButton;
                color: hifi.buttons.blue;
                colorScheme: root.assetName === "" ? hifi.colorSchemes.dark : hifi.colorSchemes.light;
                anchors.right: parent.right;
                anchors.rightMargin: 0;
                anchors.verticalCenter: parent.verticalCenter;
                height: 40;
                width: 100;
                text: "SUBMIT";
                onClicked: {
                    if (root.assetName === "" && parseInt(amountTextField.text) > parseInt(balanceText.text)) {
                        amountTextField.focus = true;
                        amountTextField.error = true;
                        amountTextFieldError.text = "<i>amount exceeds available funds</i>";
                    } else if (root.assetName === "" && (amountTextField.text === "" || parseInt(amountTextField.text) < 1)) {
                        amountTextField.focus = true;
                        amountTextField.error = true;
                        amountTextFieldError.text = "<i>invalid amount</i>";
                    } else {
                        amountTextFieldError.text = "";
                        amountTextField.error = false;
                        root.isCurrentlySendingAsset = true;
                        amountTextField.focus = false;
                        optionalMessage.focus = false;
                        if (sendAssetStep.referrer === "connections") {
                            Commerce.transferAssetToUsername(sendAssetStep.selectedRecipientUserName,
                                root.assetCertID,
                                parseInt(amountTextField.text),
                                optionalMessage.text);
                        } else if (sendAssetStep.referrer === "nearby") {
                            Commerce.transferAssetToNode(sendAssetStep.selectedRecipientNodeID,
                                root.assetCertID,
                                parseInt(amountTextField.text),
                                optionalMessage.text);
                        }
                    }
                }
            }
        }
    }
    // Send Asset Screen END
    
    // Sending Asset Overlay START
    Rectangle {
        id: sendingAssetOverlay;
        z: 999;

        visible: root.isCurrentlySendingAsset;
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
            id: sendingAssetImage;
            source: "../../common/images/loader.gif"
            width: 96;
            height: width;
            anchors.verticalCenter: parent.verticalCenter;
            anchors.horizontalCenter: parent.horizontalCenter;
        }

        RalewaySemiBold {
            text: "Sending";
            // Anchors
            anchors.top: sendingAssetImage.bottom;
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
    // Sending Asset Overlay END
    
    // Payment Success BEGIN
    Rectangle {
        id: paymentSuccess;
        z: 998;

        visible: root.currentActiveView === "paymentSuccess";
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

        Rectangle {
            anchors.top: parent.top;
            anchors.topMargin: root.assetName === "" ? 15 : 125;
            anchors.left: parent.left;
            anchors.leftMargin: root.assetName === "" ? 15 : 50;
            anchors.right: parent.right;
            anchors.rightMargin: root.assetName === "" ? 15 : 50;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: root.assetName === "" ? 15 : 125;
            color: "#FFFFFF";

            RalewaySemiBold {
                id: paymentSentText;
                text: root.assetName === "" ? "Payment Sent" : "Gift Sent";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 26;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                elide: Text.ElideRight;
                height: 30;
                // Text size
                size: 22;
                // Style
                color: hifi.colors.baseGray;
            }
            
            HiFiGlyphs {
                id: closeGlyphButton_paymentSuccess;
                visible: root.assetName === "";
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
                        root.nextActiveView = "sendAssetHome";
                        resetSendAssetData();
                        if (root.assetName !== "") {
                            sendSignalToParent({method: "closeSendAsset"});
                        }
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

                    displayName: sendAssetStep.selectedRecipientDisplayName;
                    userName: sendAssetStep.selectedRecipientUserName;
                    profilePic: sendAssetStep.selectedRecipientProfilePic !== "" ? ((0 === sendAssetStep.selectedRecipientProfilePic.indexOf("http")) ?
                        sendAssetStep.selectedRecipientProfilePic : (Account.metaverseServerURL + sendAssetStep.selectedRecipientProfilePic)) : "";
                    isDisplayingNearby: sendAssetStep.referrer === "nearby";
                }
            }
            

            Item {
                id: giftContainer_paymentSuccess;
                visible: root.assetName !== "";
                anchors.top: sendToContainer_paymentSuccess.bottom;
                anchors.topMargin: 8;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                height: 30;

                RalewaySemiBold {
                    id: gift_paymentSuccess;
                    text: "Gift:";
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

                RalewaySemiBold {
                    text: root.assetName;
                    // Anchors
                    anchors.top: parent.top;
                    anchors.left: gift_paymentSuccess.right;
                    anchors.right: parent.right;
                    height: parent.height;
                    // Text size
                    size: 18;
                    // Style
                    elide: Text.ElideRight;
                    color: hifi.colors.baseGray;
                    verticalAlignment: Text.AlignVCenter;
                }
            }

            Item {
                id: amountContainer_paymentSuccess;
                visible: root.assetName === "";
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
                visible: root.assetName === "";
                text: optionalMessage.text;
                // Anchors
                anchors.top: amountContainer_paymentSuccess.visible ? amountContainer_paymentSuccess.bottom : sendToContainer_paymentSuccess.bottom;
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
                colorScheme: root.assetName === "" ? hifi.colorSchemes.dark : hifi.colorSchemes.light;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: root.assetName === "" ? 80 : 30;
                height: 50;
                width: 120;
                text: "Close";
                onClicked: {
                    root.nextActiveView = "sendAssetHome";
                    resetSendAssetData();
                    if (root.assetName !== "") {
                        sendSignalToParent({method: "closeSendAsset"});
                    }
                }
            }
        }
    }
    // Payment Success END
    
    // Payment Failure BEGIN
    Rectangle {
        id: paymentFailure;
        z: 998;

        visible: root.currentActiveView === "paymentFailure";
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

        Rectangle {
            anchors.top: parent.top;
            anchors.topMargin: root.assetName === "" ? 15 : 150;
            anchors.left: parent.left;
            anchors.leftMargin: root.assetName === "" ? 15 : 50;
            anchors.right: parent.right;
            anchors.rightMargin: root.assetName === "" ? 15 : 50;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: root.assetName === "" ? 15 : 300;
            color: "#FFFFFF";

            RalewaySemiBold {
                id: paymentFailureText;
                text: root.assetName === "" ? "Payment Failed" : "Failed";
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 26;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                elide: Text.ElideRight;
                height: 30;
                // Text size
                size: 22;
                // Style
                color: hifi.colors.baseGray;
            }
            
            HiFiGlyphs {
                id: closeGlyphButton_paymentFailure;
                visible: root.assetName === "";
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
                        root.nextActiveView = "sendAssetHome";
                        resetSendAssetData();
                        if (root.assetName !== "") {
                            sendSignalToParent({method: "closeSendAsset"});
                        }
                    }
                }
            }

            RalewaySemiBold {
                id: paymentFailureDetailText;
                text: "The recipient you specified was unable to receive your " + (root.assetName === "" ? "payment." : "gift.");
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
                visible: root.assetName === "";
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

                    displayName: sendAssetStep.selectedRecipientDisplayName;
                    userName: sendAssetStep.selectedRecipientUserName;
                    profilePic: sendAssetStep.selectedRecipientProfilePic !== "" ? ((0 === sendAssetStep.selectedRecipientProfilePic.indexOf("http")) ?
                        sendAssetStep.selectedRecipientProfilePic : (Account.metaverseServerURL + sendAssetStep.selectedRecipientProfilePic)) : "";
                    isDisplayingNearby: sendAssetStep.referrer === "nearby";
                }
            }

            Item {
                id: amountContainer_paymentFailure;
                visible: root.assetName === "";
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
                id: optionalMessage_paymentFailure;
                visible: root.assetName === "";
                text: optionalMessage.text;
                // Anchors
                anchors.top: amountContainer_paymentFailure.visible ? amountContainer_paymentFailure.bottom : sendToContainer_paymentFailure.bottom;
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

            // "Cancel" button
            HifiControlsUit.Button {
                id: closeButton_paymentFailure;
                color: hifi.buttons.noneBorderless;
                colorScheme: root.assetName === "" ? hifi.colorSchemes.dark : hifi.colorSchemes.light;
                anchors.right: retryButton_paymentFailure.left;
                anchors.rightMargin: 12;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: root.assetName === "" ? 80 : 30;
                height: 50;
                width: 120;
                text: "Cancel";
                onClicked: {
                    root.nextActiveView = "sendAssetHome";
                    resetSendAssetData();
                    if (root.assetName !== "") {
                        sendSignalToParent({method: "closeSendAsset"});
                    }
                }
            }

            // "Retry" button
            HifiControlsUit.Button {
                id: retryButton_paymentFailure;
                color: hifi.buttons.blue;
                colorScheme: root.assetName === "" ? hifi.colorSchemes.dark : hifi.colorSchemes.light;
                anchors.right: parent.right;
                anchors.rightMargin: 12;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: root.assetName === "" ? 80 : 30;
                height: 50;
                width: 120;
                text: "Retry";
                onClicked: {
                    root.isCurrentlySendingAsset = true;
                    if (sendAssetStep.referrer === "connections") {
                        Commerce.transferAssetToUsername(sendAssetStep.selectedRecipientUserName,
                            root.assetCertID,
                            parseInt(amountTextField.text),
                            optionalMessage.text);
                    } else if (sendAssetStep.referrer === "nearby") {
                        Commerce.transferAssetToNode(sendAssetStep.selectedRecipientNodeID,
                            root.assetCertID,
                            parseInt(amountTextField.text),
                            optionalMessage.text);
                    }
                }
            }
        }
    }
    // Payment Failure END


    //
    // FUNCTION DEFINITIONS START
    //

    function resetSendAssetData() {
        amountTextField.focus = false;
        optionalMessage.focus = false;
        amountTextFieldError.text = "";
        amountTextField.error = false;
        chooseRecipientNearby.selectedRecipient = "";
        sendAssetStep.selectedRecipientNodeID = "";
        sendAssetStep.selectedRecipientDisplayName = "";
        sendAssetStep.selectedRecipientUserName = "";
        sendAssetStep.selectedRecipientProfilePic = "";
        amountTextField.text = "";
        optionalMessage.text = "";
        sendPubliclyCheckbox.checked = Settings.getValue("sendAssetsNearbyPublicly", true);
        sendAssetStep.referrer = "";
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
                    chooseRecipientNearby.selectedRecipient = message.id;
                    sendAssetStep.selectedRecipientDisplayName = message.displayName;
                    sendAssetStep.selectedRecipientUserName = message.userName;
                } else {
                    chooseRecipientNearby.selectedRecipient = "";
                    sendAssetStep.selectedRecipientDisplayName = '';
                    sendAssetStep.selectedRecipientUserName = '';
                }
            break;
            case 'updateSelectedRecipientUsername':
                sendAssetStep.selectedRecipientUserName = message.userName;
            break;
            default:
                console.log('SendAsset: Unrecognized message from wallet.js');
        }
    }
    signal sendSignalToParent(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
