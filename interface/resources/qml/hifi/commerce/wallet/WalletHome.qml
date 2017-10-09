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

    Hifi.QmlCommerce {
        id: commerce;

        onBalanceResult : {
            balanceText.text = result.data.balance;
        }

        onHistoryResult : {
            historyReceived = true;
            if (result.status === 'success') {
                transactionHistoryModel.clear();
                transactionHistoryModel.append(result.data.history);
            }
        }
    }

    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
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
                    commerce.balance();
                    commerce.history();
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
            anchors.leftMargin: 30;
            anchors.right: parent.right;
            anchors.rightMargin: 30;
            height: 30;
            // Text size
            size: 22;
            // Style
            color: hifi.colors.baseGrayHighlight;
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
            anchors.rightMargin: 24;
            
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
                    height: transactionText.height + 30;

                    HifiControlsUit.Separator {
                    visible: index === 0;
                    colorScheme: 1;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    anchors.top: parent.top;
                    }

                    AnonymousProRegular {
                        id: dateText;
                        text: getFormattedDate(model.created_at * 1000);
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
                        text: model.text;
                        size: 18;
                        anchors.top: parent.top;
                        anchors.topMargin: 15;
                        anchors.left: dateText.right;
                        anchors.leftMargin: 20;
                        anchors.right: parent.right;
                        height: paintedHeight;
                        color: hifi.colors.baseGrayHighlight;
                        wrapMode: Text.WordWrap;

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
                onAtYEndChanged: {
                    if (transactionHistory.atYEnd) {
                        console.log("User scrolled to the bottom of 'Recent Activity'.");
                        // Grab next page of results and append to model
                    }
                }
            }

            // This should never be visible (since you immediately get 100 HFC)
            FiraSansRegular {
                id: emptyTransationHistory;
                size: 24;
                visible: !transactionHistory.visible && root.historyReceived;
                text: "Recent Activity Unavailable";
                anchors.fill: parent;
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
            }
        }
    }

    //
    // FUNCTION DEFINITIONS START
    //

    function getFormattedDate(timestamp) {
        var a = new Date(timestamp);
        var year = a.getFullYear();
        var month = a.getMonth();
        var day = a.getDate();
        var hour = a.getHours();
        var drawnHour = hour;
        if (hour === 0) {
            drawnHour = 12;
        } else if (hour > 12) {
            drawnHour -= 12;
        }
        
        var amOrPm = "AM";
        if (hour >= 12) {
            amOrPm = "PM";
        }

        var min = a.getMinutes();
        var sec = a.getSeconds();
        return year + '-' + month + '-' + day + '<br>' + drawnHour + ':' + min + amOrPm;
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
