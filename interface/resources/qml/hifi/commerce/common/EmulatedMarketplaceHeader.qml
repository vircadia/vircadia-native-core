//
//  EmulatedMarketplaceHeader.qml
//  qml/hifi/commerce/common
//
//  EmulatedMarketplaceHeader
//
//  Created by Zach Fox on 2017-09-18
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.7
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    property string referrerURL: (Account.metaverseServerURL + "/marketplace?");
    readonly property int additionalDropdownHeight: usernameDropdown.height - myUsernameButton.anchors.bottomMargin;
    property alias usernameDropdownVisible: usernameDropdown.visible;

    height: mainContainer.height + additionalDropdownHeight;

    Connections {
        target: Commerce;

        onWalletStatusResult: {
            if (walletStatus === 0) {
                sendToParent({method: "needsLogIn"});
            } else if (walletStatus === 3) {
                Commerce.getSecurityImage();
            } else if (walletStatus > 3) {
                console.log("ERROR in EmulatedMarketplaceHeader.qml: Unknown wallet status: " + walletStatus);
            }
        }

        onLoginStatusResult: {
            if (!isLoggedIn) {
                sendToParent({method: "needsLogIn"});
            } else {
                Commerce.getWalletStatus();
            }
        }

        onSecurityImageResult: {
            if (exists) {
                securityImage.source = "";
                securityImage.source = "image://security/securityImage";
            }
        }
    }

    Component.onCompleted: {
        Commerce.getWalletStatus();
    }

    Connections {
        target: GlobalServices
        onMyUsernameChanged: {
            Commerce.getLoginStatus();
        }
    }

    Rectangle {
        id: mainContainer;
        color: hifi.colors.white;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;
        height: 70;

        Image {
            id: marketplaceHeaderImage;
            source: "images/marketplaceHeaderImage.png";
            anchors.top: parent.top;
            anchors.topMargin: 2;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 10;
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            width: 140;
            fillMode: Image.PreserveAspectFit;

            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    sendToParent({method: "header_marketplaceImageClicked", referrerURL: root.referrerURL});
                }
            }
        }

        Item {
            id: buttonAndUsernameContainer;
            anchors.left: marketplaceHeaderImage.right;
            anchors.leftMargin: 8;
            anchors.top: parent.top;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 10;
            anchors.right: securityImage.left;
            anchors.rightMargin: 6;

            Rectangle {
                id: myPurchasesLink;
                anchors.right: myUsernameButton.left;
                anchors.rightMargin: 8;
                anchors.verticalCenter: parent.verticalCenter;
                height: 40;
                width: myPurchasesText.paintedWidth + 10;

                RalewaySemiBold {
                    id: myPurchasesText;
                    text: "My Purchases";
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.blueAccent;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    // Anchors
                    anchors.centerIn: parent;
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: enabled;
                    onClicked: {
                        sendToParent({method: 'header_goToPurchases'});
                    }
                    onEntered: myPurchasesText.color = hifi.colors.blueHighlight;
                    onExited: myPurchasesText.color = hifi.colors.blueAccent;
                }
            }

            FontLoader { id: ralewayRegular; source: "../../../../fonts/Raleway-Regular.ttf"; }
            TextMetrics {
                id: textMetrics;
                font.family: ralewayRegular.name
                text: usernameText.text;
            }

            Rectangle {
                id: myUsernameButton;
                anchors.right: parent.right;
                anchors.verticalCenter: parent.verticalCenter;
                height: 40;
                width: usernameText.width + 25;
                color: "white";
                radius: 4;
                border.width: 1;
                border.color: hifi.colors.lightGray;

                // Username Text
                RalewayRegular {
                    id: usernameText;
                    text: Account.username;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.baseGray;
                    elide: Text.ElideRight;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    width: Math.min(textMetrics.width + 25, 110);
                    // Anchors
                    anchors.centerIn: parent;
                    rightPadding: 10;
                }

                HiFiGlyphs {
                    id: dropdownIcon;
                    text: hifi.glyphs.caratDn;
                    // Size
                    size: 50;
                    // Anchors
                    anchors.right: parent.right;
                    anchors.rightMargin: -14;
                    anchors.verticalCenter: parent.verticalCenter;
                    horizontalAlignment: Text.AlignHCenter;
                    // Style
                    color: hifi.colors.baseGray;
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: enabled;
                    onClicked: {
                        usernameDropdown.visible = !usernameDropdown.visible;
                    }
                    onEntered: usernameText.color = hifi.colors.baseGrayShadow;
                    onExited: usernameText.color = hifi.colors.baseGray;
                }
            }
        }

        Image {
            id: securityImage;
            source: "";
            visible: securityImage.source !== "";
            anchors.right: parent.right;
            anchors.rightMargin: 6;
            anchors.top: parent.top;
            anchors.topMargin: 6;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 16;
            width: height;
            mipmap: true;
            cache: false;

            MouseArea {
                enabled: securityImage.visible;
                anchors.fill: parent;
                onClicked: {
                    sendToParent({method: "showSecurityPicLightbox", securityImageSource: securityImage.source});
                }
            }
        }
    
        LinearGradient {
            z: 996;
            anchors.bottom: parent.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 10;
            start: Qt.point(0, 0);
            end: Qt.point(0, height);
            gradient: Gradient {
                GradientStop { position: 0.0; color: hifi.colors.lightGrayText }
                GradientStop { position: 1.0; color: hifi.colors.white }
            }
        }

        Item {
            id: usernameDropdown;
            z: 998;
            visible: false;
            anchors.top: buttonAndUsernameContainer.bottom;
            anchors.topMargin: -buttonAndUsernameContainer.anchors.bottomMargin;
            anchors.right: buttonAndUsernameContainer.right;
            height: childrenRect.height;
            width: 100;

            Rectangle {
                id: myItemsButton;
                color: hifi.colors.white;
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 50;

                RalewaySemiBold {
                    anchors.fill: parent;
                    text: "My Items"
                    color: hifi.colors.baseGray;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    size: 18;
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onEntered: {
                        myItemsButton.color = hifi.colors.blueHighlight;
                    }
                    onExited: {
                        myItemsButton.color = hifi.colors.white;
                    }
                    onClicked: {
                        sendToParent({method: "header_myItemsClicked"});
                    }
                }
            }

            Rectangle {
                id: logOutButton;
                color: hifi.colors.white;
                anchors.top: myItemsButton.bottom;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 50;

                RalewaySemiBold {
                    anchors.fill: parent;
                    text: "Log Out"
                    color: hifi.colors.baseGray;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    size: 18;
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onEntered: {
                        logOutButton.color = hifi.colors.blueHighlight;
                    }
                    onExited: {
                        logOutButton.color = hifi.colors.white;
                    }
                    onClicked: {
                        Account.logOut();
                    }
                }
            }
        }

        DropShadow {
            z: 997;
            visible: usernameDropdown.visible;
            anchors.fill: usernameDropdown;
            horizontalOffset: 3;
            verticalOffset: 3;
            radius: 8.0;
            samples: 17;
            color: "#80000000";
            source: usernameDropdown;
        }
    }


    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToParent(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
