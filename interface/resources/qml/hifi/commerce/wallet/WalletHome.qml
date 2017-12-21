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

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    property bool historyReceived: false;
    property int pendingCount: 0;

    Connections {
        target: Commerce;

        onBalanceResult : {
            balanceText.text = result.data.balance;
        }

        onHistoryResult : {
            historyReceived = true;
            if (result.status === 'success') {
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
            }
            refreshTimer.start();
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

            onVisibleChanged: {
                if (visible) {
                    historyReceived = false;
                    Commerce.balance();
                    Commerce.history();
                } else {
                    refreshTimer.stop();
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

    Timer {
        id: refreshTimer;
        interval: 4000;
        onTriggered: {
            console.log("Refreshing Wallet Home...");
            Commerce.balance();
            Commerce.history();
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
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            height: 30;
            // Text size
            size: 22;
            // Style
            color: hifi.colors.baseGrayHighlight;
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
                visible: transactionHistoryModel.count === 0 && root.historyReceived;
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
                text: "<b>The Wallet app is in closed Beta.</b><br><br>To request entry and <b>receive free HFC</b>, please contact " +
                "<b>info@highfidelity.com</b> with your High Fidelity account username and the email address registered to that account.";
                // Text size
                size: 24;
                // Style
                color: hifi.colors.blueAccent;
                anchors.left: parent.left;
                anchors.leftMargin: 12;
                anchors.right: parent.right;
                anchors.rightMargin: 12;
                anchors.verticalCenter: parent.verticalCenter;
                height: paintedHeight;
                wrapMode: Text.WordWrap;
                horizontalAlignment: Text.AlignHCenter;
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
                            id: dateText;
                            text: model.created_at ? getFormattedDate(model.created_at * 1000) : "";
                            // Style
                            size: 18;
                            anchors.left: parent.left;
                            anchors.top: parent.top;
                            anchors.topMargin: 15;
                            width: 118;
                            height: paintedHeight;
                            color: hifi.colors.blueAccent;
                            wrapMode: Text.WordWrap;
                            // Alignment
                            horizontalAlignment: Text.AlignRight;
                        }

                        AnonymousProRegular {
                            id: transactionText;
                            text: model.text ? (model.status === "invalidated" ? ("INVALIDATED: " + model.text) : model.text) : "";
                            size: 18;
                            anchors.top: parent.top;
                            anchors.topMargin: 15;
                            anchors.left: dateText.right;
                            anchors.leftMargin: 20;
                            anchors.right: parent.right;
                            height: paintedHeight;
                            color: model.status === "invalidated" ? hifi.colors.redAccent : hifi.colors.baseGrayHighlight;
                            wrapMode: Text.WordWrap;
                            font.strikeout: model.status === "invalidated";

                            onLinkActivated: {
                                sendSignalToWallet({method: 'transactionHistory_linkClicked', marketplaceLink: link});
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
                        // Grab next page of results and append to model
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
