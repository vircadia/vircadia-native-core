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
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.2
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls
import "qrc:////qml//hifi//models" as HifiModels  // Absolute path so the same code works everywhere.

Item {
    HifiConstants { id: hifi; }

    id: root;

    onVisibleChanged: {
        if (visible) {
            Commerce.balance();
            transactionHistoryModel.getFirstPage();
            Commerce.getAvailableUpdates();
        } else {
            refreshTimer.stop();
        }
    }

    Connections {
        target: Commerce;

        onBalanceResult : {
            balanceText.text = result.data.balance;
        }

        onHistoryResult : {
            transactionHistoryModel.handlePage(null, result);
        }

        onAvailableUpdatesResult: {
            if (result.status !== 'success') {
                console.log("Failed to get Available Updates", result.data.message);
            } else {
                sendToScript({method: 'wallet_availableUpdatesReceived', numUpdates: result.data.updates.length });
            }
        }
    }

    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            transactionHistoryModel.resetModel();
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
        color: hifi.colors.white;
        elide: Text.ElideRight;
        // Anchors
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        width: parent.width/2 - anchors.leftMargin;
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

    Timer {
        id: refreshTimer;
        interval: 6000;
        onTriggered: {
            if (transactionHistory.atYBeginning) {
                console.log("Refreshing 1st Page of Recent Activity...");
                Commerce.balance();
                transactionHistoryModel.getFirstPage("delayedClear");
            }
        }
    }

    // Recent Activity
    Rectangle {
        id: recentActivityContainer;
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
            id: recentActivityText;
            text: "Recent Activity";
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
            color: hifi.colors.baseGrayHighlight;
        }

        RalewaySemiBold {
            id: myPurchasesLink;
            text: '<font color="#0093C5"><a href="#myPurchases">My Purchases</a></font>';
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 26;
            anchors.right: parent.right;
            anchors.rightMargin: 20;
            width: paintedWidth;
            height: 30;
            y: 4;
            // Text size
            size: 18;
            // Style
            color: hifi.colors.baseGrayHighlight;
            horizontalAlignment: Text.AlignRight;

            onLinkActivated: {
                sendSignalToWallet({method: 'goToPurchases_fromWalletHome'});
            }
        }

        HifiModels.PSFListModel {
            id: transactionHistoryModel;
            property int lastPendingCount: 0;
            listModelName: "transaction history"; // For debugging. Alternatively, we could specify endpoint for that purpose, even though it's not used directly.
            listView: transactionHistory;
            itemsPerPage: 6;
            getPage: function () {
                console.debug('getPage', transactionHistoryModel.listModelName, transactionHistoryModel.currentPageToRetrieve);
                Commerce.history(transactionHistoryModel.currentPageToRetrieve, transactionHistoryModel.itemsPerPage);
            }
            processPage: function (data) {
                console.debug('processPage', transactionHistoryModel.listModelName, JSON.stringify(data));
                var result, pending; // Set up or get the accumulator for pending.
                if (transactionHistoryModel.currentPageToRetrieve === 1) {
                    // The initial data elements inside the ListModel MUST contain all keys
                    // that will be used in future data.
                    pending = {
                        transaction_type: "pendingCount",
                        count: 0,
                        created_at: 0,
                        hfc_text: "",
                        id: "",
                        message: "",
                        place_name: "",
                        received_certs: 0,
                        received_money: 0,
                        recipient_name: "",
                        sender_name: "",
                        sent_certs: 0,
                        sent_money: 0,
                        status: "",
                        transaction_text: ""
                    };
                    result = [pending];
                } else {
                    pending = transactionHistoryModel.get(0);
                    result = [];
                }

                // Either add to pending, or to result.
                // Note that you only see a page of pending stuff until you scroll...
                data.history.forEach(function (item) {
                    if (item.status === 'pending') {
                        pending.count++;
                    } else {
                        result = result.concat(item);
                    }
                });

                if (lastPendingCount === 0) {
                    lastPendingCount = pending.count;
                } else {
                    if (lastPendingCount !== pending.count) {
                        transactionHistoryModel.getNextPageIfNotEnoughVerticalResults();
                    }
                    lastPendingCount = pending.count;
                }

                // Only auto-refresh if the user hasn't scrolled
                // and there is more data to grab
                if (transactionHistory.atYBeginning && data.history.length) {
                    refreshTimer.start();
                }
                return result;
            }
        }
        Item {
            anchors.top: recentActivityText.bottom;
            anchors.topMargin: 26;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;
            
            ListView {
                id: transactionHistory;
                ScrollBar.vertical: ScrollBar {
                    policy: transactionHistory.contentHeight > parent.parent.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded;
                    parent: transactionHistory.parent;
                    anchors.top: transactionHistory.top;
                    anchors.left: transactionHistory.right;
                    anchors.leftMargin: 4;
                    anchors.bottom: transactionHistory.bottom;
                    width: 20;
                }
                anchors.centerIn: parent;
                width: parent.width - 12;
                height: parent.height;
                visible: transactionHistoryModel.count !== 0;
                clip: true;
                model: transactionHistoryModel;
                delegate: Item {
                    width: parent.width;
                    height: (model.transaction_type === "pendingCount" && model.count !== 0) ? 40 : ((model.status === "confirmed" || model.status === "invalidated") ? transactionText.height + 30 : 0);

                    Item {
                        visible: model.transaction_type === "pendingCount" && model.count !== 0;
                        anchors.top: parent.top;
                        anchors.left: parent.left;
                        width: parent.width;
                        height: visible ? parent.height : 0;

                        AnonymousProRegular {
                            id: pendingCountText;
                            anchors.fill: parent;
                            text: model.count + ' Transaction' + (model.count > 1 ? 's' : '') + ' Pending';
                            size: 18;
                            color: hifi.colors.blueAccent;
                            verticalAlignment: Text.AlignVCenter;
                            horizontalAlignment: Text.AlignHCenter;
                        }
                    }

                    Item {
                        visible: model.transaction_type !== "pendingCount" && (model.status === "confirmed" || model.status === "invalidated");
                        anchors.top: parent.top;
                        anchors.left: parent.left;
                        width: parent.width;
                        height: visible ? parent.height : 0;

                        AnonymousProRegular {
                            id: hfcText;
                            text: model.hfc_text || '';
                            // Style
                            size: 18;
                            anchors.left: parent.left;
                            anchors.top: parent.top;
                            anchors.topMargin: 15;
                            width: 118;
                            height: paintedHeight;
                            wrapMode: Text.Wrap;
                            // Alignment
                            horizontalAlignment: Text.AlignRight;
                        }

                        AnonymousProRegular {
                            id: transactionText;
                            text: model.transaction_text ? (model.status === "invalidated" ? ("INVALIDATED: " + model.transaction_text) : model.transaction_text) : "";
                            size: 18;
                            anchors.top: parent.top;
                            anchors.topMargin: 15;
                            anchors.left: hfcText.right;
                            anchors.leftMargin: 20;
                            anchors.right: parent.right;
                            height: paintedHeight;
                            color: model.status === "invalidated" ? hifi.colors.redAccent : hifi.colors.baseGrayHighlight;
                            linkColor: hifi.colors.blueAccent;
                            wrapMode: Text.Wrap;
                            font.strikeout: model.status === "invalidated";

                            onLinkActivated: {
                                if (link.indexOf("users/") !== -1) {
                                    sendSignalToWallet({method: 'transactionHistory_usernameLinkClicked', usernameLink: link});
                                } else {
                                    sendSignalToWallet({method: 'transactionHistory_linkClicked', marketplaceLink: link});
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
            }

            Item {
                // On empty history. We don't want to flash and then replace, so don't show until we know we should.
                // The history is empty when it contains 1 item (the pending item count) AND there are no pending items.
                visible: transactionHistoryModel.count === 1 &&
                    transactionHistoryModel.retrievedAtLeastOnePage &&
                    transactionHistoryModel.get(0).count === 0;
                anchors.centerIn: parent;
                width: parent.width - 12;
                height: parent.height;

                HifiControlsUit.Separator {
                    colorScheme: 1;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    anchors.top: parent.top;
                }

                RalewayRegular {
                    id: noActivityText;
                    text: "Congrats! Your wallet is all set!<br><br>" +
                        "<b>Where's my HFC?</b><br>" +
                        "High Fidelity commerce is in open beta right now. Want more HFC? Get it by meeting with a banker at " +
                        "<a href='#goToBank'>BankOfHighFidelity</a>!"
                    // Text size
                    size: 22;
                    // Style
                    color: hifi.colors.blueAccent;
                    anchors.top: parent.top;
                    anchors.topMargin: 36;
                    anchors.left: parent.left;
                    anchors.leftMargin: 12;
                    anchors.right: parent.right;
                    anchors.rightMargin: 12;
                    height: paintedHeight;
                    wrapMode: Text.WordWrap;
                    horizontalAlignment: Text.AlignHCenter;

                    onLinkActivated: {
                        sendSignalToWallet({ method: "transactionHistory_goToBank" });
                    }
                }

                HifiControlsUit.Button {
                    id: bankButton;
                    color: hifi.buttons.blue;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.top: noActivityText.bottom;
                    anchors.topMargin: 30;
                    anchors.horizontalCenter: parent.horizontalCenter;
                    width: parent.width/2;
                    height: 50;
                    text: "VISIT BANK OF HIGH FIDELITY";
                    onClicked: {
                        sendSignalToWallet({ method: "transactionHistory_goToBank" });
                    }
                }
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //

    function getFormattedDate(timestamp) {
        function addLeadingZero(n) {
            return n < 10 ? '0' + n : '' + n;
        }

        var a = new Date(timestamp);
        var year = a.getFullYear();
        var month = addLeadingZero(a.getMonth() + 1);
        var day = addLeadingZero(a.getDate());
        var hour = a.getHours();
        var drawnHour = hour;
        if (hour === 0) {
            drawnHour = 12;
        } else if (hour > 12) {
            drawnHour -= 12;
        }
        drawnHour = addLeadingZero(drawnHour);
        
        var amOrPm = "AM";
        if (hour >= 12) {
            amOrPm = "PM";
        }

        var min = addLeadingZero(a.getMinutes());
        var sec = addLeadingZero(a.getSeconds());
        return year + '-' + month + '-' + day + '<br>' + drawnHour + ':' + min + amOrPm;
    }

    signal sendSignalToWallet(var msg);

    //
    // FUNCTION DEFINITIONS END
    //
}
