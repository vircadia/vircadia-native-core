//
//  Purchases.qml
//  qml/hifi/commerce/purchases
//
//  Purchases
//
//  Created by Zach Fox on 2017-08-25
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls
import "../wallet" as HifiWallet
import "../common" as HifiCommerceCommon
import "../inspectionCertificate" as HifiInspectionCertificate
import "../common/sendAsset" as HifiSendAsset

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "initialize";
    property string referrerURL: "";
    property bool securityImageResultReceived: false;
    property bool purchasesReceived: false;
    property bool punctuationMode: false;
    property bool isShowingMyItems: false;
    property bool isDebuggingFirstUseTutorial: false;
    property int pendingItemCount: 0;
    property string installedApps;
    property bool keyboardRaised: false;
    property int numUpdatesAvailable: 0;
    // Style
    color: hifi.colors.white;
    Connections {
        target: Commerce;

        onWalletStatusResult: {
            if (walletStatus === 0) {
                if (root.activeView !== "needsLogIn") {
                    root.activeView = "needsLogIn";
                }
            } else if ((walletStatus === 1) || (walletStatus === 2) || (walletStatus === 3)) {
                if (root.activeView !== "notSetUp") {
                    root.activeView = "notSetUp";
                    notSetUpTimer.start();
                }
            } else if (walletStatus === 4) {
                if (root.activeView !== "passphraseModal") {
                    root.activeView = "passphraseModal";
                    UserActivityLogger.commercePassphraseEntry("marketplace purchases");
                }
            } else if (walletStatus === 5) {
                if ((Settings.getValue("isFirstUseOfPurchases", true) || root.isDebuggingFirstUseTutorial) && root.activeView !== "firstUseTutorial") {
                    root.activeView = "firstUseTutorial";
                } else if (!Settings.getValue("isFirstUseOfPurchases", true) && root.activeView === "initialize") {
                    root.activeView = "purchasesMain";
                    root.installedApps = Commerce.getInstalledApps();
                    Commerce.inventory();
                    Commerce.getAvailableUpdates();
                }
            } else {
                console.log("ERROR in Purchases.qml: Unknown wallet status: " + walletStatus);
            }
        }

        onLoginStatusResult: {
            if (!isLoggedIn && root.activeView !== "needsLogIn") {
                root.activeView = "needsLogIn";
            } else {
                Commerce.getWalletStatus();
            }
        }

        onInventoryResult: {
            purchasesReceived = true;

            if (result.status !== 'success') {
                console.log("Failed to get purchases", result.message);
            } else if (!purchasesContentsList.dragging) { // Don't modify the view if the user's scrolling
                var inventoryResult = processInventoryResult(result.data.assets);

                var currentIndex = purchasesContentsList.currentIndex === -1 ? 0 : purchasesContentsList.currentIndex;
                purchasesModel.clear();
                purchasesModel.append(inventoryResult);

                root.pendingItemCount = 0;
                for (var i = 0; i < purchasesModel.count; i++) {
                    if (purchasesModel.get(i).status === "pending") {
                        root.pendingItemCount++;
                    }
                }

                if (previousPurchasesModel.count !== 0) {
                    checkIfAnyItemStatusChanged();
                } else {
                    // Fill statusChanged default value
                    // Not doing this results in the default being true...
                    for (var i = 0; i < purchasesModel.count; i++) {
                        purchasesModel.setProperty(i, "statusChanged", false);
                    }
                }
                previousPurchasesModel.append(inventoryResult);

                buildFilteredPurchasesModel();

                purchasesContentsList.positionViewAtIndex(currentIndex, ListView.Beginning);
            }
        }

        onAvailableUpdatesResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Available Updates", result.data.message);
            } else {
                sendToScript({method: 'purchases_availableUpdatesReceived', numUpdates: result.data.updates.length });
                root.numUpdatesAvailable = result.data.updates.length;
            }
        }

        onAppInstalled: {
            root.installedApps = Commerce.getInstalledApps();
        }

        onAppUninstalled: {
            root.installedApps = Commerce.getInstalledApps();
        }
    }

    Timer {
        id: notSetUpTimer;
        interval: 200;
        onTriggered: {
            sendToScript({method: 'purchases_walletNotSetUp'});
        }
    }

    HifiInspectionCertificate.InspectionCertificate {
        id: inspectionCertificate;
        z: 998;
        visible: false;
        anchors.fill: parent;

        Connections {
            onSendToScript: {
                sendToScript(message);
            }
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        z: 999;
        visible: false;
        anchors.fill: parent;

        Connections {
            onSendToParent: {
                if (msg.method === 'commerceLightboxLinkClicked') {
                    Qt.openUrlExternally(msg.linkUrl);
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    HifiSendAsset.SendAsset {
        id: sendAsset;
        z: 998;
        visible: root.activeView === "giftAsset";
        anchors.fill: parent;
        parentAppTitleBarHeight: 70;
        parentAppNavBarHeight: 0;

        Connections {
            onSendSignalToParent: {
                if (msg.method === 'sendAssetHome_back' || msg.method === 'closeSendAsset') {
                    root.activeView = "purchasesMain";
                    Commerce.inventory();
                    Commerce.getAvailableUpdates();
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    //
    // TITLE BAR START
    //
    HifiCommerceCommon.EmulatedMarketplaceHeader {
        id: titleBarContainer;
        z: 997;
        visible: !needsLogIn.visible;
        // Size
        width: parent.width;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;

        Connections {
            onSendToParent: {
                if (msg.method === 'needsLogIn' && root.activeView !== "needsLogIn") {
                    root.activeView = "needsLogIn";
                } else if (msg.method === 'showSecurityPicLightbox') {
                    lightboxPopup.titleText = "Your Security Pic";
                    lightboxPopup.bodyImageSource = msg.securityImageSource;
                    lightboxPopup.bodyText = lightboxPopup.securityPicBodyText;
                    lightboxPopup.button1text = "CLOSE";
                    lightboxPopup.button1method = function() {
                        lightboxPopup.visible = false;
                    }
                    lightboxPopup.button2text = "GO TO WALLET";
                    lightboxPopup.button2method = function() {
                        sendToScript({method: 'purchases_openWallet'});
                        lightboxPopup.visible = false;
                    };
                    lightboxPopup.visible = true;
                } else {
                    sendToScript(msg);
                }
            }
        }
    }
    MouseArea {
        enabled: titleBarContainer.usernameDropdownVisible;
        anchors.fill: parent;
        onClicked: {
            titleBarContainer.usernameDropdownVisible = false;
        }
    }
    //
    // TITLE BAR END
    //

    Rectangle {
        id: initialize;
        visible: root.activeView === "initialize";
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: -titleBarContainer.additionalDropdownHeight;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        color: hifi.colors.white;

        Component.onCompleted: {
            securityImageResultReceived = false;
            purchasesReceived = false;
            Commerce.getWalletStatus();
        }
    }
        
    Item {
        id: installedAppsContainer;
        z: 998;
        visible: false;
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: -titleBarContainer.additionalDropdownHeight;
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        width: parent.width;

        RalewayRegular {
            id: installedAppsHeader;
            anchors.top: parent.top;
            anchors.topMargin: 10;
            anchors.left: parent.left;
            anchors.leftMargin: 12;
            height: 80;
            width: paintedWidth;
            text: "All Installed Marketplace Apps";
            color: hifi.colors.black;
            size: 22;
        }

        ListView {
            id: installedAppsList;
            clip: true;
            model: installedAppsModel;
            snapMode: ListView.SnapToItem;
            // Anchors
            anchors.top: installedAppsHeader.bottom;
            anchors.left: parent.left;
            anchors.bottom: sideloadAppButton.top;
            width: parent.width;
            delegate: Item {
                width: parent.width;
                height: 40;
                
                RalewayRegular {
                    text: model.appUrl;
                    // Text size
                    size: 16;
                    // Anchors
                    anchors.left: parent.left;
                    anchors.leftMargin: 12;
                    height: parent.height;
                    anchors.right: sideloadAppOpenButton.left;
                    anchors.rightMargin: 8;
                    elide: Text.ElideRight;
                    // Style
                    color: hifi.colors.black;
                    // Alignment
                    verticalAlignment: Text.AlignVCenter;

                    MouseArea {
                        anchors.fill: parent;
                        onClicked: {
                            Window.copyToClipboard((model.appUrl).slice(0, -9));
                        }
                    }
                }

                HifiControlsUit.Button {
                    id: sideloadAppOpenButton;
                    text: "OPEN";
                    color: hifi.buttons.blue;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.top: parent.top;
                    anchors.topMargin: 2;
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: 2;
                    anchors.right: uninstallGlyph.left;
                    anchors.rightMargin: 8;
                    width: 80;
                    onClicked: {
                        Commerce.openApp(model.appUrl);
                    }
                }
            
                HiFiGlyphs {
                    id: uninstallGlyph;
                    text: hifi.glyphs.close;
                    color: hifi.colors.black;
                    size: 22;
                    anchors.top: parent.top;
                    anchors.right: parent.right;
                    anchors.rightMargin: 6;
                    width: 35;
                    height: parent.height;
                    horizontalAlignment: Text.AlignHCenter;
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
                            Commerce.uninstallApp(model.appUrl);
                        }
                    }
                }
            }
        }
        HifiControlsUit.Button {
            id: sideloadAppButton;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.dark;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 8;
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.right: closeAppListButton.left;
            anchors.rightMargin: 8;
            height: 40;
            text: "SIDELOAD APP FROM LOCAL DISK";
            onClicked: {
                Window.browseChanged.connect(onFileOpenChanged); 
                Window.browseAsync("Locate your app's .app.json file", "", "*.app.json");
            }
        }
        HifiControlsUit.Button {
            id: closeAppListButton;
            color: hifi.buttons.white;
            colorScheme: hifi.colorSchemes.dark;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 8;
            anchors.right: parent.right;
            anchors.rightMargin: 8;
            width: 100;
            height: 40;
            text: "BACK";
            onClicked: {
                installedAppsContainer.visible = false;
            }
        }
    }

    HifiWallet.NeedsLogIn {
        id: needsLogIn;
        visible: root.activeView === "needsLogIn";
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        Connections {
            onSendSignalToWallet: {
                sendToScript(msg);
            }
        }
    }
    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            Commerce.getLoginStatus();
        }
    }

    HifiWallet.PassphraseModal {
        id: passphraseModal;
        visible: root.activeView === "passphraseModal";
        anchors.fill: parent;
        titleBarText: "Purchases";
        titleBarIcon: hifi.glyphs.wallet;

        Connections {
            onSendSignalToParent: {
                if (msg.method === "authSuccess") {
                    root.activeView = "initialize";
                    Commerce.getWalletStatus();
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    HifiCommerceCommon.FirstUseTutorial {
        id: firstUseTutorial;
        z: 999;
        visible: root.activeView === "firstUseTutorial";
        anchors.fill: parent;

        Connections {
            onSendSignalToParent: {
                switch (message.method) {
                    case 'tutorial_skipClicked':
                    case 'tutorial_finished':
                        Settings.setValue("isFirstUseOfPurchases", false);
                        root.activeView = "purchasesMain";
                        root.installedApps = Commerce.getInstalledApps();
                        Commerce.inventory();
                        Commerce.getAvailableUpdates();
                    break;
                }
            }
        }
    }

    //
    // PURCHASES CONTENTS START
    //
    Item {
        id: purchasesContentsContainer;
        visible: root.activeView === "purchasesMain" && !installedAppsList.visible;
        // Anchors
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: titleBarContainer.bottom;
        anchors.topMargin: 8 - titleBarContainer.additionalDropdownHeight;
        anchors.bottom: parent.bottom;

        //
        // FILTER BAR START
        //
        Item {
            z: 996;
            id: filterBarContainer;
            // Size
            height: 40;
            // Anchors
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.right: parent.right;
            anchors.rightMargin: 8;
            anchors.top: parent.top;
            anchors.topMargin: 4;

            RalewayRegular {
                id: myText;
                anchors.top: parent.top;
                anchors.topMargin: 10;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 10;
                anchors.left: parent.left;
                anchors.leftMargin: 16;
                width: paintedWidth;
                text: isShowingMyItems ? "My Items" : "My Purchases";
                color: hifi.colors.black;
                size: 22;
            }

            HifiControlsUit.FilterBar {
                id: filterBar;
                property string previousText: "";
                property string previousPrimaryFilter: "";
                colorScheme: hifi.colorSchemes.faintGray;
                anchors.top: parent.top;
                anchors.right: parent.right;
                anchors.rightMargin: 8;
                anchors.left: myText.right;
                anchors.leftMargin: 16;
                textFieldHeight: 39;
                height: textFieldHeight + dropdownHeight;
                placeholderText: "filter items";

                Component.onCompleted: {
                    var choices = [
                        {
                            "displayName": "App",
                            "filterName": "app"
                        },
                        {
                            "displayName": "Avatar",
                            "filterName": "avatar"
                        },
                        {
                            "displayName": "Content Set",
                            "filterName": "contentSet"
                        },
                        {
                            "displayName": "Entity",
                            "filterName": "entity"
                        },
                        {
                            "displayName": "Wearable",
                            "filterName": "wearable"
                        },
                        {
                            "displayName": "Updatable",
                            "filterName": "updatable"
                        }
                    ]
                    filterBar.primaryFilterChoices.clear();
                    filterBar.primaryFilterChoices.append(choices);
                }

                onPrimaryFilter_displayNameChanged: {
                    buildFilteredPurchasesModel();
                    purchasesContentsList.positionViewAtIndex(0, ListView.Beginning)
                    filterBar.previousPrimaryFilter = filterBar.primaryFilter_displayName;
                }

                onTextChanged: {
                    buildFilteredPurchasesModel();
                    purchasesContentsList.positionViewAtIndex(0, ListView.Beginning)
                    filterBar.previousText = filterBar.text;
                }
            }
        }
        //
        // FILTER BAR END
        //

        HifiControlsUit.Separator {
            z: 995;
            id: separator;
            colorScheme: 2;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 16;
        }

        ListModel {
            id: purchasesModel;
        }
        ListModel {
            id: previousPurchasesModel;
        }
        HifiCommerceCommon.SortableListModel {
            id: tempPurchasesModel;
        }
        HifiCommerceCommon.SortableListModel {
            id: filteredPurchasesModel;
        }

        ListView {
            id: purchasesContentsList;
            visible: (root.isShowingMyItems && filteredPurchasesModel.count !== 0) || (!root.isShowingMyItems && filteredPurchasesModel.count !== 0);
            clip: true;
            model: filteredPurchasesModel;
            snapMode: ListView.SnapToItem;
            // Anchors
            anchors.top: separator.bottom;
            anchors.left: parent.left;
            anchors.bottom: updatesAvailableBanner.visible ? updatesAvailableBanner.top : parent.bottom;
            width: parent.width;
            delegate: PurchasedItem {
                itemName: title;
                itemId: id;
                itemPreviewImageUrl: preview;
                itemHref: download_url;
                certificateId: certificate_id;
                purchaseStatus: status;
                itemEdition: model.edition_number;
                numberSold: model.number_sold;
                limitedRun: model.limited_run;
                displayedItemCount: model.displayedItemCount;
                cardBackVisible: model.cardBackVisible;
                isInstalled: model.isInstalled;
                wornEntityID: model.wornEntityID;
                upgradeUrl: model.upgrade_url;
                upgradeTitle: model.upgrade_title;
                itemType: model.itemType;
                isShowingMyItems: root.isShowingMyItems;
                anchors.topMargin: 10;
                anchors.bottomMargin: 10;

                Connections {
                    onSendToPurchases: {
                        if (msg.method === 'purchases_itemInfoClicked') {
                            sendToScript({method: 'purchases_itemInfoClicked', itemId: itemId});
                        } else if (msg.method === "purchases_rezClicked") {
                            sendToScript({method: 'purchases_rezClicked', itemHref: itemHref, itemType: itemType});

                            // Race condition - Wearable might not be rezzed by the time the "currently worn wearbles" model is created
                            if (itemType === "wearable") {
                                sendToScript({ method: 'purchases_updateWearables' });
                            }
                        } else if (msg.method === 'purchases_itemCertificateClicked') {
                            inspectionCertificate.visible = true;
                            inspectionCertificate.isLightbox = true;
                            sendToScript(msg);
                        } else if (msg.method === "showInvalidatedLightbox") {
                            lightboxPopup.titleText = "Item Invalidated";
                            lightboxPopup.bodyText = 'Your item is marked "invalidated" because this item has been suspended ' +
                            "from the Marketplace due to a claim against its author.";
                            lightboxPopup.button1text = "CLOSE";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.visible = true;
                        } else if (msg.method === "showPendingLightbox") {
                            lightboxPopup.titleText = "Item Pending";
                            lightboxPopup.bodyText = 'Your item is marked "pending" while your purchase is being confirmed. ' +
                            "Usually, purchases take about 90 seconds to confirm.";
                            lightboxPopup.button1text = "CLOSE";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.visible = true;
                        } else if (msg.method === "showReplaceContentLightbox") {
                            lightboxPopup.titleText = "Replace Content";
                            lightboxPopup.bodyText = "Rezzing this content set will replace the existing environment and all of the items in this domain. " +
                                "If you want to save the state of the content in this domain, create a backup before proceeding.<br><br>" +
                                "For more information about backing up and restoring content, " +
                                "<a href='https://docs.highfidelity.com/create-and-explore/start-working-in-your-sandbox/restoring-sandbox-content'>" +
                                "click here to open info on your desktop browser.";
                            lightboxPopup.button1text = "CANCEL";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.button2text = "CONFIRM";
                            lightboxPopup.button2method = function() {
                                Commerce.replaceContentSet(msg.itemHref, msg.certID);
                                lightboxPopup.visible = false;
                            };
                            lightboxPopup.visible = true;
                        } else if (msg.method === "showChangeAvatarLightbox") {
                            lightboxPopup.titleText = "Change Avatar";
                            lightboxPopup.bodyText = "This will change your current avatar to " + msg.itemName + " while retaining your wearables.";
                            lightboxPopup.button1text = "CANCEL";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.button2text = "CONFIRM";
                            lightboxPopup.button2method = function() {
                                MyAvatar.useFullAvatarURL(msg.itemHref);
                                lightboxPopup.visible = false;
                            };
                            lightboxPopup.visible = true;
                        } else if (msg.method === "showPermissionsExplanation") {
                            if (msg.itemType === "entity") {
                                lightboxPopup.titleText = "Rez Certified Permission";
                                lightboxPopup.bodyText = "You don't have permission to rez certified items in this domain.<br><br>" +
                                    "Use the <b>GOTO app</b> to visit another domain or <b>go to your own sandbox.</b>";
                                lightboxPopup.button2text = "OPEN GOTO";
                                lightboxPopup.button2method = function() {
                                    sendToScript({method: 'purchases_openGoTo'});
                                    lightboxPopup.visible = false;
                                };
                            } else if (msg.itemType === "contentSet") {
                                lightboxPopup.titleText = "Replace Content Permission";
                                lightboxPopup.bodyText = "You do not have the permission 'Replace Content' in this <b>domain's server settings</b>. The domain owner " +
                                    "must enable it for you before you can replace content sets in this domain.";
                            }
                            lightboxPopup.button1text = "CLOSE";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.visible = true;
                        } else if (msg.method === "setFilterText") {
                            filterBar.text = msg.filterText;
                        } else if (msg.method === "flipCard") {
                            for (var i = 0; i < filteredPurchasesModel.count; i++) {
                                if (i !== index || msg.closeAll) {
                                    filteredPurchasesModel.setProperty(i, "cardBackVisible", false);
                                } else {
                                    filteredPurchasesModel.setProperty(i, "cardBackVisible", true);
                                }
                            }
                        } else if (msg.method === "updateItemClicked") {
                            sendToScript(msg);
                        } else if (msg.method === "giftAsset") {
                            sendAsset.assetName = msg.itemName;
                            sendAsset.assetCertID = msg.certId;
                            sendAsset.sendingPubliclyEffectImage = msg.effectImage;

                            if (msg.itemType === "avatar" && MyAvatar.skeletonModelURL === msg.itemHref) {
                                lightboxPopup.titleText = "Change Avatar to Default";
                                lightboxPopup.bodyText = "You are currently wearing the avatar that you are trying to gift.<br><br>" +
                                "If you proceed, your avatar will be changed to the default avatar.";
                                lightboxPopup.button1text = "CANCEL";
                                lightboxPopup.button1method = function() {
                                    lightboxPopup.visible = false;
                                }
                                lightboxPopup.button2text = "CONFIRM";
                                lightboxPopup.button2method = function() {
                                    MyAvatar.skeletonModelURL = '';
                                    root.activeView = "giftAsset";
                                    lightboxPopup.visible = false;
                                };
                                lightboxPopup.visible = true;
                            } else if (msg.itemType === "app" && msg.isInstalled) {
                                lightboxPopup.titleText = "Uninstall App";
                                lightboxPopup.bodyText = "You are currently using the app that you are trying to gift.<br><br>" +
                                "If you proceed, the app will be uninstalled.";
                                lightboxPopup.button1text = "CANCEL";
                                lightboxPopup.button1method = function() {
                                    lightboxPopup.visible = false;
                                }
                                lightboxPopup.button2text = "CONFIRM";
                                lightboxPopup.button2method = function() {
                                    Commerce.uninstallApp(msg.itemHref);
                                    root.activeView = "giftAsset";
                                    lightboxPopup.visible = false;
                                };
                                lightboxPopup.visible = true;
                            } else if (msg.itemType === "wearable" && msg.wornEntityID !== '') {
                                lightboxPopup.titleText = "Remove Wearable";
                                lightboxPopup.bodyText = "You are currently wearing the wearable that you are trying to send.<br><br>" +
                                "If you proceed, this wearable will be removed.";
                                lightboxPopup.button1text = "CANCEL";
                                lightboxPopup.button1method = function() {
                                    lightboxPopup.visible = false;
                                }
                                lightboxPopup.button2text = "CONFIRM";
                                lightboxPopup.button2method = function() {
                                    Entities.deleteEntity(msg.wornEntityID);
                                    filteredPurchasesModel.setProperty(index, 'wornEntityID', '');
                                    root.activeView = "giftAsset";
                                    lightboxPopup.visible = false;
                                };
                                lightboxPopup.visible = true;
                            } else {
                                root.activeView = "giftAsset";
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: updatesAvailableBanner;
            visible: root.numUpdatesAvailable > 0 && !root.isShowingMyItems;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 75;
            color: "#B5EAFF";
            
            Rectangle {
                id: updatesAvailableGlyph;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.left: parent.left;
                anchors.leftMargin: 16;
                // Size
                width: 10;
                height: width;
                radius: width/2;
                // Style
                color: "red";
            }

            RalewaySemiBold {
                text: "You have " + root.numUpdatesAvailable + " item updates available.";
                // Text size
                size: 18;
                // Anchors
                anchors.left: updatesAvailableGlyph.right;
                anchors.leftMargin: 12;
                height: parent.height;
                width: paintedWidth;
                // Style
                color: hifi.colors.black;
                // Alignment
                verticalAlignment: Text.AlignVCenter;
            }

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: true;
                propagateComposedEvents: false;
            }

            HifiControlsUit.Button {
                color: hifi.buttons.white;
                colorScheme: hifi.colorSchemes.dark;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.right: parent.right;
                anchors.rightMargin: 12;
                width: 100;
                height: 40;
                text: "SHOW ME";
                onClicked: {
                    filterBar.text = "";
                    filterBar.changeFilterByDisplayName("Updatable");
                }
            }
        }

        Item {
            id: noItemsAlertContainer;
            visible: !purchasesContentsList.visible &&
                root.purchasesReceived &&
                root.isShowingMyItems &&
                filterBar.text === "" &&
                filterBar.primaryFilter_displayName === "";
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;

            // Explanitory text
            RalewayRegular {
                id: noItemsYet;
                text: "<b>You haven't submitted anything to the Marketplace yet!</b><br><br>Submit an item to the Marketplace to add it to My Items.";
                // Text size
                size: 22;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 150;
                anchors.left: parent.left;
                anchors.leftMargin: 24;
                anchors.right: parent.right;
                anchors.rightMargin: 24;
                height: paintedHeight;
                // Style
                color: hifi.colors.baseGray;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
            }

            // "Go To Marketplace" button
            HifiControlsUit.Button {
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: noItemsYet.bottom;
                anchors.topMargin: 20;
                anchors.horizontalCenter: parent.horizontalCenter;
                width: parent.width * 2 / 3;
                height: 50;
                text: "Visit Marketplace";
                onClicked: {
                    sendToScript({method: 'purchases_goToMarketplaceClicked'});
                }
            }
        }

        Item {
            id: noPurchasesAlertContainer;
            visible: !purchasesContentsList.visible &&
                root.purchasesReceived &&
                !root.isShowingMyItems &&
                filterBar.text === "" &&
                filterBar.primaryFilter_displayName === "";
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;

            // Explanitory text
            RalewayRegular {
                id: haventPurchasedYet;
                text: "<b>You haven't purchased anything yet!</b><br><br>Get an item from <b>Marketplace</b> to add it to My Purchases.";
                // Text size
                size: 22;
                // Anchors
                anchors.top: parent.top;
                anchors.topMargin: 150;
                anchors.left: parent.left;
                anchors.leftMargin: 24;
                anchors.right: parent.right;
                anchors.rightMargin: 24;
                height: paintedHeight;
                // Style
                color: hifi.colors.baseGray;
                wrapMode: Text.WordWrap;
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
            }

            // "Go To Marketplace" button
            HifiControlsUit.Button {
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: haventPurchasedYet.bottom;
                anchors.topMargin: 20;
                anchors.horizontalCenter: parent.horizontalCenter;
                width: parent.width * 2 / 3;
                height: 50;
                text: "Visit Marketplace";
                onClicked: {
                    sendToScript({method: 'purchases_goToMarketplaceClicked'});
                }
            }
        }
    }
    //
    // PURCHASES CONTENTS END
    //

    HifiControlsUit.Keyboard {
        id: keyboard;
        z: 999;
        raised: HMD.mounted && parent.keyboardRaised;
        numeric: parent.punctuationMode;
        anchors {
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //

    function processInventoryResult(inventory) {
        for (var i = 0; i < inventory.length; i++) {
            if (inventory[i].status.length > 1) {
                console.log("WARNING: Inventory result index " + i + " has a status of length >1!")
            }
            inventory[i].status = inventory[i].status[0];
            inventory[i].categories = inventory[i].categories.join(';');
        }
        return inventory;
    }

    function populateDisplayedItemCounts() {
        var itemCountDictionary = {};
        var currentItemId;
        for (var i = 0; i < filteredPurchasesModel.count; i++) {
            currentItemId = filteredPurchasesModel.get(i).id;
            if (itemCountDictionary[currentItemId] === undefined) {
                itemCountDictionary[currentItemId] = 1;
            } else {
                itemCountDictionary[currentItemId]++;
            }
        }

        for (var i = 0; i < filteredPurchasesModel.count; i++) {
            filteredPurchasesModel.setProperty(i, "displayedItemCount", itemCountDictionary[filteredPurchasesModel.get(i).id]);
        }
    }

    function sortByDate() {
        filteredPurchasesModel.sortColumnName = "purchase_date";
        filteredPurchasesModel.isSortingDescending = true;
        filteredPurchasesModel.valuesAreNumerical = true;
        filteredPurchasesModel.quickSort();
    }

    function buildFilteredPurchasesModel() {
        var sameItemCount = 0;
        
        tempPurchasesModel.clear();

        for (var i = 0; i < purchasesModel.count; i++) {
            if (purchasesModel.get(i).title.toLowerCase().indexOf(filterBar.text.toLowerCase()) !== -1) {
                if (!purchasesModel.get(i).valid) {
                    continue;
                }

                if (purchasesModel.get(i).status !== "confirmed" && !root.isShowingMyItems) {
                    tempPurchasesModel.insert(0, purchasesModel.get(i));
                } else if ((root.isShowingMyItems && purchasesModel.get(i).edition_number === "0") ||
                (!root.isShowingMyItems && purchasesModel.get(i).edition_number !== "0")) {
                    tempPurchasesModel.append(purchasesModel.get(i));
                }
            }
        }
        
        // primaryFilter filtering and adding of itemType property to model
        var currentItemType, currentRootFileUrl, currentCategories;
        for (var i = 0; i < tempPurchasesModel.count; i++) {
            currentRootFileUrl = tempPurchasesModel.get(i).root_file_url;
            currentCategories = tempPurchasesModel.get(i).categories;

            if (currentRootFileUrl.indexOf(".fst") > -1) {
                currentItemType = "avatar";
            } else if (currentCategories.indexOf("Wearables") > -1) {
                currentItemType = "wearable";
            } else if (currentRootFileUrl.endsWith('.json.gz') || currentRootFileUrl.endsWith('.content.zip')) {
                currentItemType = "contentSet";
            } else if (currentRootFileUrl.endsWith('.app.json')) {
                currentItemType = "app";
            } else if (currentRootFileUrl.endsWith('.json')) {
                currentItemType = "entity";
            } else {
                currentItemType = "unknown";
            }
            if (filterBar.primaryFilter_displayName !== "" &&
                ((filterBar.primaryFilter_displayName === "Updatable" && tempPurchasesModel.get(i).upgrade_url === "") ||
                (filterBar.primaryFilter_displayName !== "Updatable" && filterBar.primaryFilter_filterName.toLowerCase() !== currentItemType.toLowerCase()))) {
                tempPurchasesModel.remove(i);
                i--;
            } else {
                tempPurchasesModel.setProperty(i, 'itemType', currentItemType);
            }
        }
        
        for (var i = 0; i < tempPurchasesModel.count; i++) {
            if (!filteredPurchasesModel.get(i)) {
                sameItemCount = -1;
                break;
            } else if (tempPurchasesModel.get(i).itemId === filteredPurchasesModel.get(i).itemId &&
            tempPurchasesModel.get(i).edition_number === filteredPurchasesModel.get(i).edition_number &&
            tempPurchasesModel.get(i).status === filteredPurchasesModel.get(i).status) {
                sameItemCount++;
            }
        }

        if (sameItemCount !== tempPurchasesModel.count ||
            filterBar.text !== filterBar.previousText ||
            filterBar.primaryFilter !== filterBar.previousPrimaryFilter) {
            filteredPurchasesModel.clear();
            var currentId;
            for (var i = 0; i < tempPurchasesModel.count; i++) {
                currentId = tempPurchasesModel.get(i).id;
                
                if (!purchasesModel.get(i).valid) {
                    continue;
                }
                filteredPurchasesModel.append(tempPurchasesModel.get(i));
                filteredPurchasesModel.setProperty(i, 'cardBackVisible', false);
                filteredPurchasesModel.setProperty(i, 'isInstalled', ((root.installedApps).indexOf(currentId) > -1));
                filteredPurchasesModel.setProperty(i, 'wornEntityID', '');
            }
            
            sendToScript({ method: 'purchases_updateWearables' });
            populateDisplayedItemCounts();
            sortByDate();
        }
    }

    function checkIfAnyItemStatusChanged() {
        var currentPurchasesModelId, currentPurchasesModelEdition, currentPurchasesModelStatus;
        var previousPurchasesModelStatus;
        for (var i = 0; i < purchasesModel.count; i++) {
            currentPurchasesModelId = purchasesModel.get(i).id;
            currentPurchasesModelEdition = purchasesModel.get(i).edition_number;
            currentPurchasesModelStatus = purchasesModel.get(i).status;

            for (var j = 0; j < previousPurchasesModel.count; j++) {
                previousPurchasesModelStatus = previousPurchasesModel.get(j).status;
                if (currentPurchasesModelId === previousPurchasesModel.get(j).id &&
                    currentPurchasesModelEdition === previousPurchasesModel.get(j).edition_number &&
                    currentPurchasesModelStatus !== previousPurchasesModelStatus) {
                    
                    purchasesModel.setProperty(i, "statusChanged", true);
                } else {
                    purchasesModel.setProperty(i, "statusChanged", false);
                }
            }
        }
    }
    
    function updateCurrentlyWornWearables(wearables) {
        for (var i = 0; i < filteredPurchasesModel.count; i++) {
            for (var j = 0; j < wearables.length; j++) {
                if (filteredPurchasesModel.get(i).itemType === "wearable" &&
                    wearables[j].entityCertID === filteredPurchasesModel.get(i).certificate_id &&
                    wearables[j].entityEdition.toString() === filteredPurchasesModel.get(i).edition_number) {
                    filteredPurchasesModel.setProperty(i, 'wornEntityID', wearables[j].entityID);
                    break;
                }
            }
        }
    }

    Keys.onPressed: {
        if ((event.key == Qt.Key_F) && (event.modifiers & Qt.ControlModifier)) {
            installedAppsContainer.visible = !installedAppsContainer.visible;
            console.log("User changed visibility of installedAppsContainer to " + installedAppsContainer.visible);
        }
    }
    function onFileOpenChanged(filename) {
        // disconnect the event, otherwise the requests will stack up
        try { // Not all calls to onFileOpenChanged() connect an event.
            Window.browseChanged.disconnect(onFileOpenChanged);
        } catch (e) {
            console.log('Purchases.qml ignoring', e);
        }
        if (filename) {
            Commerce.installApp(filename);
        }
    }
    ListModel {
        id: installedAppsModel;
    }
    onInstalledAppsChanged: {
        installedAppsModel.clear();
        var installedAppsArray = root.installedApps.split(",");
        var installedAppsObject = [];
        // "- 1" because the last app string ends with ","
        for (var i = 0; i < installedAppsArray.length - 1; i++) {
            installedAppsObject[i] = {
                "appUrl": installedAppsArray[i]
            }
        }
        installedAppsModel.append(installedAppsObject);
    }

    //
    // Function Name: fromScript()
    //
    // Relevant Variables:
    // None
    //
    // Arguments:
    // message: The message sent from the JavaScript, in this case the Marketplaces JavaScript.
    //     Messages are in format "{method, params}", like json-rpc.
    //
    // Description:
    // Called when a message is received from a script.
    //
    function fromScript(message) {
        switch (message.method) {
            case 'updatePurchases':
                referrerURL = message.referrerURL || "";
                titleBarContainer.referrerURL = message.referrerURL;
                filterBar.text = message.filterText ? message.filterText : "";
            break;
            case 'inspectionCertificate_setCertificateId':
                inspectionCertificate.fromScript(message);
            break;
            case 'purchases_showMyItems':
                root.isShowingMyItems = true;
            break;
            case 'updateConnections':
                sendAsset.updateConnections(message.connections);
            break;
            case 'selectRecipient':
            case 'updateSelectedRecipientUsername':
                sendAsset.fromScript(message);
            break;
            case 'updateWearables':
                updateCurrentlyWornWearables(message.wornWearables);
            break;
            default:
                console.log('Unrecognized message from marketplaces.js:', JSON.stringify(message));
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
