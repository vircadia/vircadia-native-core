//
//  marketplace.js
//
//  Created by Roxanne Skelly on 1/18/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var AppUi = Script.require('appUi');

var BUTTON_NAME = "MARKET";
var MARKETPLACE_QML_SOURCE = "hifi/commerce/marketplace/Marketplace.qml";
var ui;
function startup() {

    ui = new AppUi({
        buttonName: BUTTON_NAME,
        sortOrder: 10,
        home: MARKETPLACE_QML_SOURCE
    });
}

function shutdown() {
}

//
// Run the functions.
//
startup();
Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
