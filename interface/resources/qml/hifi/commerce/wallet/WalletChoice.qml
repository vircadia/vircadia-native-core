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
            console.log("FIXME CREATE");
        }
    }
    RalewayRegular {
        text: "What's this?";
    }
    function onFileOpenChanged(filename) {
        console.log('fixme selected', filename);
        // disconnect the event, otherwise the requests will stack up
        try {
            // Not all calls to onFileOpenChanged() connect an event.
            Window.browseChanged.disconnect(onFileOpenChanged);
        } catch (e) {
            console.log('ignoring', e);
        }
        console.log('fixme do something with', filename);
    }
    function walletChooser() {
        console.log("GOT CLICK");
        Window.browseChanged.connect(onFileOpenChanged); 
        Window.browseAsync("Choose wallet file", "", "*.hifikey");
    }
}
