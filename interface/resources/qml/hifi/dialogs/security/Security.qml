//
//  Security.qml
//  qml\hifi\dialogs\security
//
//  Security
//
//  Created by Zach Fox on 2018-10-31
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtGraphicalEffects 1.0
import "qrc:////qml//styles-uit" as HifiStylesUit
import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//controls" as HifiControls
import "qrc:////qml//hifi//commerce//common" as HifiCommerceCommon

// references XXX from root context

Rectangle {
    HifiStylesUit.HifiConstants { id: hifi; }

    id: root;
    color: hifi.colors.darkGray;
    
    property string title: "Security Settings";
    property bool walletSetUp;

    Connections {
        target: Commerce;

        onWalletStatusResult: {
            if (walletStatus === 5) {
                Commerce.getSecurityImage();
                root.walletSetUp = true;
            } else {
                root.walletSetUp = false;
            }
        }

        onSecurityImageResult: {
            if (exists) {
                currentSecurityPicture.source = "";
                currentSecurityPicture.source = "image://security/securityImage";
            }
        }
    }

    HifiCommerceCommon.CommerceLightbox {
        id: lightboxPopup;
        visible: false;
        anchors.fill: parent;
    }

    // Username Text
    HifiStylesUit.RalewayRegular {
        id: usernameText;
        text: Account.username === "" ? Account.username : "Please Log In";
        // Text size
        size: 24;
        // Style
        color: hifi.colors.white;
        elide: Text.ElideRight;
        // Anchors
        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.leftMargin: 20;
        anchors.right: parent.right;
        anchors.rightMargin: 20;
        height: 80;
    }

    Item {
        id: pleaseLogInContainer;
        visible: Account.username === "";
        anchors.top: usernameText.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        HifiStylesUit.RalewayRegular {
            text: "Please log in for security settings."
            // Text size
            size: 24;
            // Style
            color: hifi.colors.white;
            // Anchors
            anchors.bottom: openLoginButton.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
            height: 80;
        }
        
        HifiControlsUit.Button {
            id: openLoginButton;
            color: hifi.buttons.white;
            colorScheme: hifi.colorSchemes.dark;
            anchors.centerIn: parent;
            width: 140;
            height: 40;
            text: "Change";
            onClicked: {
                DialogsManager.showLoginDialog();
            }
        }
    }

    Item {
        id: securitySettingsContainer;
        visible: !pleaseLogInContainer.visible;
        anchors.top: usernameText.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        Item {
            id: accountContainer;
            anchors.top: securitySettingsContainer.top;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: childrenRect.height;

            Rectangle {
                id: accountHeaderContainer;
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 80;
                color: hifi.colors.baseGrayHighlight;

                HifiStylesUit.RalewaySemiBold {
                    text: "Account";
                    anchors.fill: parent;
                    anchors.leftMargin: 20;
                }
            }

            Item {
                id: keepMeLoggedInContainer;
                anchors.top: accountHeaderContainer.bottom;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 80;            

                HifiControlsUit.CheckBox {
                    id: autoLogoutCheckbox;
                    checked: Settings.getValue("keepMeLoggedIn", false);
                    text: "Keep Me Logged In"
                    // Anchors
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: parent.left;
                    anchors.leftMargin: 20;
                    boxSize: 28;
                    labelFontSize: 18;
                    color: hifi.colors.white;
                    onCheckedChanged: {
                        Settings.setValue("keepMeLoggedIn", checked);
                        if (checked) {
                            Settings.setValue("keepMeLoggedIn/savedUsername", Account.username);
                        } else {
                            Settings.setValue("keepMeLoggedIn/savedUsername", "");
                        }
                    }
                }

                HifiStylesUit.RalewaySemiBold {
                    id: autoLogoutHelp;
                    text: '[?]';
                    // Anchors
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.right: autoLogoutCheckbox.right;
                    width: 30;
                    height: 30;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.blueHighlight;

                    MouseArea {
                        anchors.fill: parent;
                        hoverEnabled: true;
                        onEntered: {
                            parent.color = hifi.colors.blueAccent;
                        }
                        onExited: {
                            parent.color = hifi.colors.blueHighlight;
                        }
                        onClicked: {
                            lightboxPopup.titleText = "Keep Me Logged In";
                            lightboxPopup.bodyText = "If you choose to stay logged in, ensure that this is a trusted device.\n\n" +
                                "Also, remember that logging out may not disconnect you from a domain.";
                            lightboxPopup.button1text = "OK";
                            lightboxPopup.button1method = function() {
                                lightboxPopup.visible = false;
                            }
                            lightboxPopup.visible = true;
                        }
                    }
                }
            }
        }

        Item {
            id: walletContainer;
            anchors.top: accountContainer.bottom;
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: childrenRect.height;

            Rectangle {
                id: walletHeaderContainer;
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 80;
                color: hifi.colors.baseGrayHighlight;

                HifiStylesUit.RalewaySemiBold {
                    text: "Wallet";
                    anchors.fill: parent;
                    anchors.leftMargin: 20;
                }
            }

            Item {
                id: walletSecurityPictureContainer;
                visible: root.walletSetUp;
                anchors.top: walletHeaderContainer.bottom;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 80;

                Image {
                    id: currentSecurityPicture;
                    source: "";
                    visible: true;
                    anchors.left: parent.left;
                    anchors.leftMargin: 20;
                    anchors.verticalCenter: parent.verticalCenter;
                    height: 40;
                    width: height;
                    mipmap: true;
                    cache: false;
                }

                HifiStylesUit.RalewayRegular {
                    id: securityPictureText;
                    text: "Wallet Security Picture";
                    // Anchors
                    anchors.top: parent.top;
                    anchors.bottom: parent.bottom;
                    anchors.left: currentSecurityPicture.right;
                    anchors.leftMargin: 12;
                    width: paintedWidth;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.white;
                }

                // "Change Security Pic" button
                HifiControlsUit.Button {
                    id: changeSecurityImageButton;
                    color: hifi.buttons.white;
                    colorScheme: hifi.colorSchemes.dark;
                    anchors.left: securityPictureText.right;
                    anchors.verticalCenter: parent.verticalCenter;
                    width: 140;
                    height: 40;
                    text: "Change";
                    onClicked: {
                    
                    }
                }
            }

            Item {
                id: walletNotSetUpContainer;
                visible: !root.walletSetUp;
                anchors.top: walletHeaderContainer.bottom;
                anchors.left: parent.left;
                anchors.right: parent.right;
                height: 80;

                HifiStylesUit.RalewayRegular {
                    text: "Your wallet is not set up.\n" +
                        "Open the WALLET app to get started.";
                    // Anchors
                    anchors.fill: parent;
                    // Text size
                    size: 18;
                    // Style
                    color: hifi.colors.white;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                }
            }
        }
    }
}
