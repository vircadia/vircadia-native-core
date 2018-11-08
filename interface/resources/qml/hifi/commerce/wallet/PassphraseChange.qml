//
//  PassphraseChange.qml
//  qml/hifi/commerce/wallet
//
//  PassphraseChange
//
//  Created by Zach Fox on 2017-08-18
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Item {
    HifiConstants { id: hifi; }

    id: root;
    
    // This will cause a bug -- if you bring up passphrase selection in HUD mode while
    // in HMD while having HMD preview enabled, then move, then finish passphrase selection,
    // HMD preview will stay off.
    // TODO: Fix this unlikely bug
    onVisibleChanged: {
        if (visible) {
            sendSignalToWallet({method: 'disableHmdPreview'});
        } else {
            sendSignalToWallet({method: 'maybeEnableHmdPreview'});
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

    //
    // SECURE PASSPHRASE SELECTION START
    //
    Item {
        id: choosePassphraseContainer;
        // Anchors
        anchors.top: usernameText.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;

        // "Change Passphrase" text
        RalewaySemiBold {
            id: passphraseTitle;
            text: "Change Passphrase:";
            // Text size
            size: 18;
            anchors.top: parent.top;
            anchors.left: parent.left;
            anchors.leftMargin: 20;
            anchors.right: parent.right;
            height: 30;
            // Style
            color: hifi.colors.blueHighlight;
        }

        PassphraseSelection {
            id: passphraseSelection;
            isChangingPassphrase: true;
            anchors.top: passphraseTitle.bottom;
            anchors.topMargin: 8;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: passphraseNavBar.top;

            Connections {
                onSendMessageToLightbox: {
                    if (msg.method === 'statusResult') {
                        if (msg.status) {
                            // Success submitting new passphrase
                            sendSignalToWallet({method: "walletSecurity_changePassphraseSuccess"});
                        } else {
                            // Error submitting new passphrase
                            resetSubmitButton();
                            passphraseSelection.setErrorText("Current passphrase incorrect - try again");
                        }
                    } else {
                        sendSignalToWallet(msg);
                    }
                }
            }
        }

        // Navigation Bar
        Item {
            id: passphraseNavBar;
            // Size
            width: parent.width;
            height: 40;
            // Anchors:
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: 30;

            // "Cancel" button
            HifiControlsUit.Button {
                color: hifi.buttons.noneBorderlessWhite;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: 150;
                text: "Cancel"
                onClicked: {
                    sendSignalToWallet({method: "walletSecurity_changePassphraseCancelled"});
                }
            }

            // "Submit" button
            HifiControlsUit.Button {
                id: passphraseSubmitButton;
                color: hifi.buttons.blue;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: 150;
                text: "Submit";
                onClicked: {
                    passphraseSubmitButton.text = "Submitting...";
                    passphraseSubmitButton.enabled = false;
                    if (!passphraseSelection.validateAndSubmitPassphrase()) {
                        resetSubmitButton();
                    }
                }
            }
        }
    }
    //
    // SECURE PASSPHRASE SELECTION END
    //

    signal sendSignalToWallet(var msg);

    function resetSubmitButton() {
        passphraseSubmitButton.enabled = true;
        passphraseSubmitButton.text = "Submit";
    }

    function clearPassphraseFields() {
        passphraseSelection.clearPassphraseFields();
    }
}
