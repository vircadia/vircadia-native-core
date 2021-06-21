//
//  Purchases.qml
//  qml/hifi/commerce/purchases
//
//  Purchases
//
//  Created by Zach Fox on 2017-08-25
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.9
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.
import "../wallet" as HifiWallet
import "../common" as HifiCommerceCommon
import "../common/sendAsset" as HifiSendAsset
import "../.." as HifiCommon

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "initialize";
    property bool securityImageResultReceived: false;
    property bool purchasesReceived: false;
    property bool punctuationMode: false;
    property bool isDebuggingFirstUseTutorial: false;
    property bool isStandalone: false;
    property string installedApps;
    property bool keyboardRaised: false;
    property int numUpdatesAvailable: 0;
    // Style
    color: hifi.colors.white;
    function getPurchases() {
        root.activeView = "purchasesMain";
        root.installedApps = Commerce.getInstalledApps();
        purchasesModel.getFirstPage();
        Commerce.getAvailableUpdates();
    }

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
                    getPurchases();
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
            purchasesModel.handlePage(result.status !== "success" && result.message, result);
        }

        onAvailableUpdatesResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Available Updates", result.data.message);
            } else {
                root.numUpdatesAvailable = result.total_entries;
            }
        }

        onAppInstalled: {
            root.installedApps = Commerce.getInstalledApps(appID);
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

    Component.onCompleted: {
        isStandalone = PlatformInfo.isStandalone();
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

    HifiCommon.RootHttpRequest {
        id: http;
    }

    HifiSendAsset.SendAsset {
        id: sendAsset;
        http: http;
        listModelName: "Gift Connections";
        z: 998;
        visible: root.activeView === "giftAsset";
        keyboardContainer: root;
        anchors.fill: parent;
        parentAppTitleBarHeight: 70;
        parentAppNavBarHeight: 0;

        Connections {
            onSendSignalToParent: {
                if (msg.method === 'sendAssetHome_back' || msg.method === 'closeSendAsset') {
                    getPurchases();
                } else {
                    sendToScript(msg);
                }
            }
        }
    }

    Rectangle {
        id: initialize;
        visible: root.activeView === "initialize";
        anchors.top: parent.top;
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
        anchors.top: parent.top;
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
                        getPurchases();
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
        anchors.top: parent.top;
        anchors.topMargin: 8;
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
                text: "Items";
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
                            "filterName": "content_set"
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
                            "separator" : true,
                            "displayName": "Updatable",
                            "filterName": "updated"
                        },
                        {
                            "displayName": "My Submissions",
                            "filterName": "proofs"
                        }
                    ]
                    filterBar.primaryFilterChoices.clear();
                    filterBar.primaryFilterChoices.append(choices);
                }

                onPrimaryFilter_displayNameChanged: {
                    purchasesModel.tagsFilter = filterBar.primaryFilter_filterName;
                    filterBar.previousPrimaryFilter = filterBar.primaryFilter_displayName;
                }

                onTextChanged: {
                    purchasesModel.searchFilter = filterBar.text;
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

        HifiModels.PSFListModel {
            id: purchasesModel;
            itemsPerPage: 7;
            listModelName: 'purchases';
            listView: purchasesContentsList;
            getPage: function () {
                console.debug('getPage', purchasesModel.listModelName, filterBar.primaryFilter_filterName, purchasesModel.currentPageToRetrieve, purchasesModel.itemsPerPage);
                var editionFilter = "";
                var primaryFilter = "";

                if (filterBar.primaryFilter_filterName === "proofs") {
                    editionFilter = "proofs";
                } else {
                    primaryFilter = filterBar.primaryFilter_filterName;
                }
                Commerce.inventory(
                    editionFilter,
                    primaryFilter,
                    filterBar.text,
                    purchasesModel.currentPageToRetrieve,
                    purchasesModel.itemsPerPage
                );
            }
            processPage: function(data) {
                purchasesReceived = true; // HRS FIXME?
                data.assets.forEach(function (item) {
                    if (item.status.length > 1) { console.warn("Unrecognized inventory status", item); }
                    item.status = item.status[0];
                    item.categories = item.categories.join(';');
                    item.cardBackVisible = false;
                    item.isInstalled = root.installedApps.indexOf(item.id) > -1;
                    item.wornEntityID = '';
                    item.upgrade_id = item.upgrade_id ? item.upgrade_id : "";
                });
                sendToScript({ method: 'purchases_updateWearables' });
                return data.assets;
            }
        }

        ListView {
            id: purchasesContentsList;
            visible: purchasesModel.count !== 0;
            interactive: !lightboxPopup.visible;
            clip: true;
            model: purchasesModel;
            snapMode: ListView.NoSnap;
            // Anchors
            anchors.top: separator.bottom;
            anchors.left: parent.left;
            anchors.bottom: updatesAvailableBanner.visible ? updatesAvailableBanner.top : parent.bottom;
            width: parent.width;
            delegate: PurchasedItem {
                itemName: title;
                itemId: id;
                updateItemId: model.upgrade_id
                itemPreviewImageUrl: preview;
                itemHref: download_url;
                certificateId: certificate_id;
                purchaseStatus: status;
                itemEdition: model.edition_number;
                numberSold: model.number_sold;
                limitedRun: model.limited_run;
                displayedItemCount: 999; // For now (and maybe longer), we're going to display all the edition numbers.
                cardBackVisible: model.cardBackVisible || false;
                isInstalled: model.isInstalled || false;
                wornEntityID: model.wornEntityID;
                upgradeTitle: model.upgrade_title;
                itemType: model.item_type;
                valid: model.valid;
                standaloneOptimized: model.standalone_optimized
                standaloneIncompatible: root.isStandalone && model.standalone_incompatible
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
                            sendToScript(msg);
                        } else if (msg.method === "showInvalidatedLightbox") {
                            lightboxPopup.titleText = "Item Invalidated";
                            lightboxPopup.bodyText = 'This item has been invalidated and is no longer available.<br>' +
                                'If you have questions, please contact marketplace@highfidelity.com.<br>' +
                                'Thank you!';
                            lightboxPopup.button1text = "CLOSE";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.visible = true;
                        } else if (msg.method === "showPendingLightbox") {
                            lightboxPopup.titleText = "Item Pending";
                            lightboxPopup.bodyText = 'Your item is marked "pending" while the transfer is being confirmed. ' +
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
                                "<a href='https://docs.vircadia.com/host/maintain-domain/backup-domain.html'>" +
                                "click here to open info on your desktop browser.";
                            lightboxPopup.button1text = "CANCEL";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.button2text = "CONFIRM";
                            lightboxPopup.button2method = function() {
                                Commerce.replaceContentSet(msg.itemHref, msg.certID, msg.itemName);
                                lightboxPopup.visible = false;
                            };
                            lightboxPopup.visible = true;
                        } else if (msg.method === "showTrashLightbox") {
                            lightboxPopup.titleText = "Send \"" + msg.itemName + "\" to Trash";
                            lightboxPopup.bodyText = "Sending this item to the Trash means you will no longer own this item " +
                                "and it will be inaccessible to you from Inventory.\n\nThis action cannot be undone.";
                            lightboxPopup.button1text = "CANCEL";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.button2text = "CONFIRM";
                            lightboxPopup.button2method = function() {
                                if (msg.isInstalled) {
                                    Commerce.uninstallApp(msg.itemHref);
                                }

                                if (MyAvatar.skeletonModelURL === msg.itemHref) {
                                    MyAvatar.useFullAvatarURL('');
                                }

                                if (msg.itemType === "wearable" && msg.wornEntityID !== '') {
                                    Entities.deleteEntity(msg.wornEntityID);
                                    purchasesModel.setProperty(index, 'wornEntityID', '');
                                }

                                Commerce.transferAssetToUsername("trashbot", msg.certID, 1, "Sent " + msg.itemName + " to trash.");

                                lightboxPopup.titleText = '"' + msg.itemName + '" Sent to Trash';
                                lightboxPopup.button1text = "OK";
                                lightboxPopup.button1method = function() {
                                    root.purchasesReceived = false;
                                    lightboxPopup.visible = false;
                                    getPurchases();
                                }
                                lightboxPopup.button2text = "";
                                lightboxPopup.bodyText = "";
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
                        } else if (msg.method === "showStandaloneIncompatibleExplanation") {
                            lightboxPopup.titleText = "Stand-alone Incompatible";
                            lightboxPopup.bodyText = "The item is incompatible with stand-alone devices.";
                            lightboxPopup.button1text = "CLOSE";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.visible = true;
                        } else if (msg.method === "setFilterText") {
                            filterBar.text = msg.filterText;
                        } else if (msg.method === "flipCard") {
                            for (var i = 0; i < purchasesModel.count; i++) {
                                if (i !== index || msg.closeAll) {
                                    purchasesModel.setProperty(i, "cardBackVisible", false);
                                } else {
                                    purchasesModel.setProperty(i, "cardBackVisible", true);
                                }
                            }
                        } else if (msg.method === "updateItemClicked") {
                            // These three cases are very similar to the conditionals below, under
                            // "if msg.method === "giftAsset". They differ in their popup's wording
                            // and the actions to take when continuing.
                            // I could see an argument for DRYing up this code, but I think the
                            // actions are different enough now and potentially moving forward such that I'm
                            // OK with "somewhat repeating myself".
                            if (msg.itemType === "app" && msg.isInstalled) {
                                lightboxPopup.titleText = "Uninstall App";
                                lightboxPopup.bodyText = "The app that you are trying to update is installed.<br><br>" +
                                "If you proceed, the current version of the app will be uninstalled.";
                                lightboxPopup.button1text = "CANCEL";
                                lightboxPopup.button1method = function() {
                                    lightboxPopup.visible = false;
                                }
                                lightboxPopup.button2text = "CONFIRM";
                                lightboxPopup.button2method = function() {
                                    Commerce.uninstallApp(msg.itemHref);
                                    sendToScript(msg);
                                };
                                lightboxPopup.visible = true;
                            } else if (msg.itemType === "wearable" && msg.wornEntityID !== '') {
                                lightboxPopup.titleText = "Remove Wearable";
                                lightboxPopup.bodyText = "You are currently wearing the wearable that you are trying to update.<br><br>" +
                                "If you proceed, this wearable will be removed.";
                                lightboxPopup.button1text = "CANCEL";
                                lightboxPopup.button1method = function() {
                                    lightboxPopup.visible = false;
                                }
                                lightboxPopup.button2text = "CONFIRM";
                                lightboxPopup.button2method = function() {
                                    Entities.deleteEntity(msg.wornEntityID);
                                    purchasesModel.setProperty(index, 'wornEntityID', '');
                                    sendToScript(msg);
                                };
                                lightboxPopup.visible = true;
                            } else if (msg.itemType === "avatar" && MyAvatar.skeletonModelURL === msg.itemHref) {
                                lightboxPopup.titleText = "Change Avatar to Default";
                                lightboxPopup.bodyText = "You are currently wearing the avatar that you are trying to update.<br><br>" +
                                "If you proceed, your avatar will be changed to the default avatar.";
                                lightboxPopup.button1text = "CANCEL";
                                lightboxPopup.button1method = function() {
                                    lightboxPopup.visible = false;
                                }
                                lightboxPopup.button2text = "CONFIRM";
                                lightboxPopup.button2method = function() {
                                    MyAvatar.useFullAvatarURL('');
                                    sendToScript(msg);
                                };
                                lightboxPopup.visible = true;
                            } else {
                                sendToScript(msg);
                            }
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
                                    MyAvatar.useFullAvatarURL('');
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
                                    purchasesModel.setProperty(index, 'wornEntityID', '');
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
            visible: root.numUpdatesAvailable > 0 &&
                     filterBar.primaryFilter_filterName !== "proofs";
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
                filterBar.text === "" &&
                filterBar.primaryFilter_filterName === "proofs";
            anchors.top: filterBarContainer.bottom;
            anchors.topMargin: 12;
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            width: parent.width;

            // Explanitory text
            RalewayRegular {
                id: noItemsYet;
                text: "<b>You haven't submitted anything to the Marketplace yet!</b><br><br>Submit an item to the Marketplace to add it to My Submissions.";
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
                text: "<b>You haven't gotten anything yet!</b><br><br>Get an item from <b>Marketplace</b> to add it to your Inventory.";
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

    function updateCurrentlyWornWearables(wearables) {
        for (var i = 0; i < purchasesModel.count; i++) {
            for (var j = 0; j < wearables.length; j++) {
                if (purchasesModel.get(i).item_type === "wearable" &&
                    wearables[j].entityCertID === purchasesModel.get(i).certificate_id &&
                    wearables[j].entityEdition.toString() === purchasesModel.get(i).edition_number) {
                    purchasesModel.setProperty(i, 'wornEntityID', wearables[j].entityID);
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
                filterBar.text = message.filterText ? message.filterText : "";
            break;
            case 'purchases_showMyItems':
                filterBar.primaryFilter_filterName = "proofs";
                filterBar.primaryFilter_displayName = "Proofs";
                filterBar.primaryFilter_index = 6;
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
            case 'http.response':
                http.handleHttpResponse(message);
            break;
            default:
                console.log('Purchases.qml: Unrecognized message from marketplaces.js');
        }
    }
    signal sendToScript(var message);

    //
    // FUNCTION DEFINITIONS END
    //
}
