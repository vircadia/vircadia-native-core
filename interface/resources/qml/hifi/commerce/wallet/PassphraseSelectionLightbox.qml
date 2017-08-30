//
//  PassphraseSelectionLightbox.qml
//  qml/hifi/commerce/wallet
//
//  PassphraseSelectionLightbox
//
//  Created by Zach Fox on 2017-08-18
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit
import "../../../controls" as HifiControls

// references XXX from root context

Rectangle {
    HifiConstants { id: hifi; }

    id: root;
    // Style
    color: hifi.colors.baseGray;

    //
    // SECURE PASSPHRASE SELECTION START
    //
    Item {
        id: choosePassphraseContainer;
        // Anchors
        anchors.fill: parent;

        Item {
            id: passphraseTitle;
            // Size
            width: parent.width;
            height: 50;
            // Anchors
            anchors.left: parent.left;
            anchors.top: parent.top;

            // Title Bar text
            RalewaySemiBold {
                text: "CHANGE PASSPHRASE";
                // Text size
                size: hifi.fontSizes.overlayTitle;
                // Anchors
                anchors.top: parent.top;
                anchors.left: parent.left;
                anchors.leftMargin: 16;
                anchors.bottom: parent.bottom;
                width: paintedWidth;
                // Style
                color: hifi.colors.faintGray;
                // Alignment
                horizontalAlignment: Text.AlignHLeft;
                verticalAlignment: Text.AlignVCenter;
            }
        }

        // Text below title bar
        RalewaySemiBold {
            id: passphraseTitleHelper;
            text: "Choose a Secure Passphrase";
            // Text size
            size: 24;
            // Anchors
            anchors.top: passphraseTitle.bottom;
            anchors.left: parent.left;
            anchors.leftMargin: 16;
            anchors.right: parent.right;
            anchors.rightMargin: 16;
            height: 50;
            // Style
            color: hifi.colors.faintGray;
            // Alignment
            horizontalAlignment: Text.AlignHLeft;
            verticalAlignment: Text.AlignVCenter;
        }

        PassphraseSelection {
            id: passphraseSelection;
            anchors.top: passphraseTitleHelper.bottom;
            anchors.topMargin: 30;
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: passphraseNavBar.top;

            Connections {
                onSendMessageToLightbox: {
                    if (msg.method === 'statusResult') {
                        if (msg.status) {
                            // Success submitting new passphrase
                            root.resetSubmitButton();
                            root.visible = false;
                        } else {
                            // Error submitting new passphrase
                            root.resetSubmitButton();
                            passphraseSelection.setErrorText("Backend error");
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
            height: 100;
            // Anchors:
            anchors.left: parent.left;
            anchors.bottom: parent.bottom;

            // "Cancel" button
            HifiControlsUit.Button {
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.left: parent.left;
                anchors.leftMargin: 20;
                width: 100;
                text: "Cancel"
                onClicked: {
                    root.visible = false;
                }
            }

            // "Submit" button
            HifiControlsUit.Button {
                id: passphraseSubmitButton;
                color: hifi.buttons.black;
                colorScheme: hifi.colorSchemes.dark;
                anchors.top: parent.top;
                anchors.topMargin: 3;
                anchors.bottom: parent.bottom;
                anchors.bottomMargin: 3;
                anchors.right: parent.right;
                anchors.rightMargin: 20;
                width: 100;
                text: "Submit";
                onClicked: {
                    if (passphraseSelection.validateAndSubmitPassphrase()) {
                        passphraseSubmitButton.text = "Submitting...";
                        passphraseSubmitButton.enabled = false;
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
