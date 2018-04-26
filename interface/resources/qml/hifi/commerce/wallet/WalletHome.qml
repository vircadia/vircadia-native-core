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

Item {
    HifiConstants { id: hifi; }

    id: root;
    property bool initialHistoryReceived: false;
    property bool historyRequestPending: true;
    property bool noMoreHistoryData: false;
    property int pendingCount: 0;
    property int currentHistoryPage: 1;
    property var pagesAlreadyAdded: new Array();

    onVisibleChanged: {
        if (visible) {
            transactionHistoryModel.clear();
            Commerce.balance();
            initialHistoryReceived = false;
            root.currentHistoryPage = 1;
            root.noMoreHistoryData = false;
            root.historyRequestPending = true;
            Commerce.history(root.currentHistoryPage);
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
            root.initialHistoryReceived = true;
            root.historyRequestPending = false;

            if (result.status === 'success') {
                var currentPage = parseInt(result.current_page);

                if (result.data.history.length === 0) {
                    root.noMoreHistoryData = true;
                    console.log("No more data to retrieve from Commerce.history() endpoint.")
                } else if (root.currentHistoryPage === 1) {
                    var sameItemCount = 0;
                    tempTransactionHistoryModel.clear();
                
                    tempTransactionHistoryModel.append(result.data.history);
        
                    for (var i = 0; i < tempTransactionHistoryModel.count; i++) {
                        if (!transactionHistoryModel.get(i)) {
                            sameItemCount = -1;
                            break;
                        } else if (tempTransactionHistoryModel.get(i).transaction_type === transactionHistoryModel.get(i).transaction_type &&
                        tempTransactionHistoryModel.get(i).text === transactionHistoryModel.get(i).text) {
                            sameItemCount++;
                        }
                    }

                    if (sameItemCount !== tempTransactionHistoryModel.count) {
                        transactionHistoryModel.clear();
                        for (var i = 0; i < tempTransactionHistoryModel.count; i++) {
                            transactionHistoryModel.append(tempTransactionHistoryModel.get(i));
                        }
                        calculatePendingAndInvalidated();
                    }
                } else {
                    if (root.pagesAlreadyAdded.indexOf(currentPage) !== -1) {
                        console.log("Page " + currentPage + " of history has already been added to the list.");
                    } else {
                        // First, add the history result to a temporary model
                        tempTransactionHistoryModel.clear();
                        tempTransactionHistoryModel.append(result.data.history);

                        // Make a note that we've already added this page to the model...
                        root.pagesAlreadyAdded.push(currentPage);

                        var insertionIndex = 0;
                        // If there's nothing in the model right now, we don't need to modify insertionIndex.
                        if (transactionHistoryModel.count !== 0) {
                            var currentIteratorPage;
                            // Search through the whole transactionHistoryModel and look for the insertion point.
                            // The insertion point is found when the result page from the server is less than
                            //     the page that the current item came from, OR when we've reached the end of the whole model.
                            for (var i = 0; i < transactionHistoryModel.count; i++) {
                                currentIteratorPage = transactionHistoryModel.get(i).resultIsFromPage;
                        
                                if (currentPage < currentIteratorPage) {
                                    insertionIndex = i;
                                    break;
                                } else if (i === transactionHistoryModel.count - 1) {
                                    insertionIndex = i + 1;
                                    break;
                                }
                            }
                        }
                    
                        // Go through the results we just got back from the server, setting the "resultIsFromPage"
                        //     property of those results and adding them to the main model.
                        for (var i = 0; i < tempTransactionHistoryModel.count; i++) {
                            tempTransactionHistoryModel.setProperty(i, "resultIsFromPage", currentPage);
                            transactionHistoryModel.insert(i + insertionIndex, tempTransactionHistoryModel.get(i))
                        }

                        calculatePendingAndInvalidated();
                    }
                }
            }

            // Only auto-refresh if the user hasn't scrolled
            // and there is more data to grab
            if (transactionHistory.atYBeginning && !root.noMoreHistoryData) {
                refreshTimer.start();
            }
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
            transactionHistoryModel.clear();
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

    Timer {
        id: refreshTimer;
        interval: 4000;
        onTriggered: {
            if (transactionHistory.atYBeginning) {
                console.log("Refreshing 1st Page of Recent Activity...");
                root.historyRequestPending = true;
                Commerce.balance();
                Commerce.history(1);
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

        ListModel {
            id: tempTransactionHistoryModel;
        }
        ListModel {
            id: transactionHistoryModel;
        }
        Item {
            anchors.top: recentActivityText.bottom;
            anchors.topMargin: 26;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;

            Item {
                visible: transactionHistoryModel.count === 0 && root.initialHistoryReceived;
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
                    height: (model.transaction_type === "pendingCount" && root.pendingCount !== 0) ? 40 : ((model.status === "confirmed" || model.status === "invalidated") ? transactionText.height + 30 : 0);

                    Item {
                        visible: model.transaction_type === "pendingCount" && root.pendingCount !== 0;
                        anchors.top: parent.top;
                        anchors.left: parent.left;
                        width: parent.width;
                        height: visible ? parent.height : 0;

                        AnonymousProRegular {
                            id: pendingCountText;
                            anchors.fill: parent;
                            text: root.pendingCount + ' Transaction' + (root.pendingCount > 1 ? 's' : '') + ' Pending';
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
                onAtYEndChanged: {
                    if (transactionHistory.atYEnd) {
                        console.log("User scrolled to the bottom of 'Recent Activity'.");
                        if (!root.historyRequestPending && !root.noMoreHistoryData) {
                            // Grab next page of results and append to model
                            root.historyRequestPending = true;
                            Commerce.history(++root.currentHistoryPage);
                            console.log("Fetching Page " + root.currentHistoryPage + " of Recent Activity...");
                        }
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

    
    function calculatePendingAndInvalidated(startingPendingCount) {
        var pendingCount = startingPendingCount ? startingPendingCount : 0;
        for (var i = 0; i < transactionHistoryModel.count; i++) {
            if (transactionHistoryModel.get(i).status === "pending") {
                pendingCount++;
            }
        }

        root.pendingCount = pendingCount;
        if (pendingCount > 0) {
            transactionHistoryModel.insert(0, {"transaction_type": "pendingCount"});
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
