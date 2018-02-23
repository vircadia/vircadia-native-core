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
import "../../../styles-uit"
import "../../../controls-uit" as HifiControlsUit


Column {
    property string activeView: "conflict";
    property bool isMissing: activeView === "preeexisting";
    property var proceedFunction: nil;
    RalewayBold {
        text: isMissing 
            ? "Where are your private keys?"
            : "Hmmm, your keys are different";
    }
    RalewayRegular {
        text: isMissing 
            ? "Our records indicate that you created a wallet but the private keys are not in the folder where we checked."
            : "Our records indicate that you created a wallet with different keys than the keys you're providing."
   }
    HifiControlsUit.Button {
        text: isMissing
            ? "LOCATE MY KEYS"
            : "LOCATE OTHER KEYS";
        color: hifi.buttons.blue;
        colorScheme: hifi.colorSchemes.dark;
        onClicked: walletChooser();
    }
    HifiControlsUit.Button {
        text: isMissing
            ? "CREATE NEW WALLET"
            : "CONTINUE WITH THESE KEYS"
        color: hifi.buttons.blue;
        colorScheme: hifi.colorSchemes.dark;
        onClicked: {
            console.log("FIXME open modal that says Are you sure, where the yes button calls proceed directly instead of this next line.")
            proceed(true);
        }
    }
    RalewayRegular {
        text: "What's this?";
    }

    function onFileOpenChanged(filename) {
        // disconnect the event, otherwise the requests will stack up
        try { // Not all calls to onFileOpenChanged() connect an event.
            Window.browseChanged.disconnect(onFileOpenChanged);
        } catch (e) {
            console.log('WalletChoice.qml ignoring', e);
        }
        if (filename) {
            console.log("FIXME copy file to the right place");
            proceed(false);
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
}
