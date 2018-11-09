//
//  WalletChoice.qml
//  qml/hifi/commerce/wallet
//
//  WalletChoice
//
//  Created by Howard Stearns
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../common" as HifiCommerceCommon
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit


Item {
    HifiConstants { id: hifi; }

    id: root;
    property string activeView: "conflict";
    property var proceedFunction: nil;
    property var copyFunction: nil;
    property string referrer: "";

    Image {
        anchors.fill: parent;
        source: "images/wallet-bg.jpg";
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    //
    // TITLE BAR START
    //
    Item {
        id: titleBarContainer;
        // Size
        height: 50;
        // Anchors
        anchors.left: parent.left;
        anchors.top: parent.top;
        anchors.right: parent.right;

        // Wallet icon
        HiFiGlyphs {
            id: walletIcon;
            text: hifi.glyphs.wallet;
            // Size
            size: parent.height * 0.8;
            // Anchors
            anchors.left: parent.left;
            anchors.leftMargin: 8;
            anchors.verticalCenter: parent.verticalCenter;
            // Style
            color: hifi.colors.blueHighlight;
        }

        // Title Bar text
        RalewayRegular {
            id: titleBarText;
            text: "Wallet Setup";
            // Text size
            size: hifi.fontSizes.overlayTitle;
            // Anchors
            anchors.top: parent.top;
            anchors.left: walletIcon.right;
            anchors.leftMargin: 8;
            anchors.bottom: parent.bottom;
            width: paintedWidth;
            // Style
            color: hifi.colors.white;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }
    }
    //
    // TITLE BAR END
    //

    //
    // MAIN PAGE START
    //
    Item {
        id: preexistingContainer;
        // Anchors
        anchors.top: titleBarContainer.bottom;
        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;

        HiFiGlyphs {
            id: bigKeyIcon;
            text: hifi.glyphs.walletKey;
            // Size
            size: 180;
            // Anchors
            anchors.top: parent.top;
            anchors.topMargin: 40;
            anchors.horizontalCenter: parent.horizontalCenter;
            // Style
            color: hifi.colors.white;
        }

        RalewayRegular {
            id: text01;
            text: root.activeView === "preexisting" ? 
                "Where are your private keys?" :
                "Hmm, your keys are different"
            // Text size
            size: 26;
            // Anchors
            anchors.top: bigKeyIcon.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: paintedHeight;
            width: paintedWidth;
            // Style
            color: hifi.colors.white;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }

        RalewayRegular {
            id: text02;
            text: root.activeView === "preexisting" ? 
                "Our records indicate that you created a wallet, but the private keys are not in the folder where we checked." :
                "Our records indicate that you created a wallet with different keys than the keys you're providing."
            // Text size
            size: 18;
            // Anchors
            anchors.top: text01.bottom;
            anchors.topMargin: 40;
            anchors.left: parent.left;
            anchors.leftMargin: 65;
            anchors.right: parent.right;
            anchors.rightMargin: 65;
            height: paintedHeight;
            width: paintedWidth;
            // Style
            color: hifi.colors.white;
            wrapMode: Text.WordWrap;
            // Alignment
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }

        // "Locate" button
        HifiControlsUit.Button {
            id: locateButton;
            color: hifi.buttons.blue;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: text02.bottom;
            anchors.topMargin: 40;
            anchors.horizontalCenter: parent.horizontalCenter;
            width: parent.width/2;
            height: 50;
            text: root.activeView === "preexisting" ? 
                "LOCATE MY KEYS" :
                "LOCATE OTHER KEYS"
            onClicked: {
                walletChooser();
            }
        }

        // "Create New" OR "Continue" button
        HifiControlsUit.Button {
            id: button02;
            color: hifi.buttons.none;
            colorScheme: hifi.colorSchemes.dark;
            anchors.top: locateButton.bottom;
            anchors.topMargin: 20;
            anchors.horizontalCenter: parent.horizontalCenter;
            width: parent.width/2;
            height: 50;
            text: root.activeView === "preexisting" ? 
                "CREATE NEW WALLET" :
                "CONTINUE WITH THESE KEYS"
            onClicked: {
                    lightboxPopup.titleText = "Are you sure?";
                    lightboxPopup.bodyText = "Taking this step will abandon your old wallet and you will no " +
                        "longer be able to access your money and your past purchases.<br><br>" +
                        "This step should only be used if you cannot find your keys.<br><br>" +
                        "This step cannot be undone.";
                    lightboxPopup.button1color = hifi.buttons.red;
                    lightboxPopup.button1text = "YES, CREATE NEW WALLET";
                    lightboxPopup.button1method = function() {
                        lightboxPopup.visible = false;
                        proceed(true);
                    }
                    lightboxPopup.button2text = "CANCEL";
                    lightboxPopup.button2method = function() {
                        lightboxPopup.visible = false;
                    };
                    lightboxPopup.buttonLayout = "topbottom";
                    lightboxPopup.visible = true;
            }
        }

        // "What's This?" link
        RalewayRegular {
            id: whatsThisLink;
            text: '<font color="#FFFFFF"><a href="#whatsthis">What\'s this?</a></font>';
            // Anchors
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 48;
            anchors.horizontalCenter: parent.horizontalCenter;
            width: paintedWidth;
            height: paintedHeight;
            // Text size
            size: 18;
            // Style
            color: hifi.colors.white;

            MouseArea {
                anchors.fill: parent;

                onClicked: {
                    if (root.activeView === "preexisting") {
                        lightboxPopup.titleText = "Your wallet's private keys are not in the folder we expected";
                        lightboxPopup.bodyText = "We see that you have created a wallet but the private keys " +
                            "for it seem to have been moved to a different folder.<br><br>" +
                            "To tell us where the keys are, click 'Locate My Keys'. <br><br>" +
                            "If you'd prefer to create a new wallet (not recommended - you will lose your money and past " +
                            "purchases), click 'Create New Wallet'.";
                        lightboxPopup.button1text = "CLOSE";
                        lightboxPopup.button1method = function() {
                            lightboxPopup.visible = false;
                        }
                        lightboxPopup.visible = true;
                    } else {
                        lightboxPopup.titleText = "You may have set up more than one wallet";
                        lightboxPopup.bodyText = "We see that the private keys stored on your computer are different " +
                            "from the ones you used last time. This may mean that you set up more than one wallet. " +
                            "If you would like to use these keys, click 'Continue With These Keys'.<br><br>" +
                            "If you would prefer to use another wallet, click 'Locate Other Keys' to show us where " +
                            "you've stored the private keys for that wallet.";
                        lightboxPopup.button1text = "CLOSE";
                        lightboxPopup.button1method = function() {
                            lightboxPopup.visible = false;
                        }
                        lightboxPopup.visible = true;
                    }
                }
            }
        }
    }
    //
    // MAIN PAGE END
    //
    
    //
    // FUNCTION DEFINITIONS START
    //
    function onFileOpenChanged(filename) {
        // disconnect the event, otherwise the requests will stack up
        try { // Not all calls to onFileOpenChanged() connect an event.
            Window.browseChanged.disconnect(onFileOpenChanged);
        } catch (e) {
            console.log('WalletChoice.qml ignoring', e);
        }
        if (filename) {
            if (copyFunction && copyFunction(filename)) {
                proceed(false);
             } else {
                console.log("WalletChoice.qml copyFunction", copyFunction, "failed.");
             }
        } // Else we're still at WalletChoice
    }
    function walletChooser() {
        Window.browseChanged.connect(onFileOpenChanged); 
        Window.browseAsync("Locate your .hifikey file", "", "*.hifikey");
    }
    function proceed(isReset) {
        if (!proceedFunction) {
            console.log("Provide a function of no arguments to WalletChoice.qml.");
        } else {
            proceedFunction(isReset);
        }
    }
    //
    // FUNCTION DEFINITIONS END
    //
}
