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
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;

    height: mainContainer.height;

    Connections {
        target: Commerce;

        onWalletStatusResult: {
            if (walletStatus === 0) {
                sendToParent({method: "needsLogIn"});
            } else if (walletStatus === 5) {
                Commerce.getSecurityImage();
            } else if (walletStatus > 5) {
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
                    sendToParent({method: "header_marketplaceImageClicked"});
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

    }


    //
    // FUNCTION DEFINITIONS START
    //
    signal sendToParent(var msg);
    //
    // FUNCTION DEFINITIONS END
    //
}
