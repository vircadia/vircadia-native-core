"use strict";
//
//  stats.js
//  scripts/system/
//
//  Created by Sam Gondelman on 3/14/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var statsbar;
var statsButton;

function init() {
    statsbar = new QmlFragment({
        qml: "hifi/StatsBar.qml"
    });

    statsButton = statsbar.addButton({
        icon: "icons/stats.svg",
        activeIcon: "icons/stats.svg",
        textSize: 45,
        bgOpacity: 0.0,
        activeBgOpacity: 0.0,
        bgColor: "#FFFFFF",
        text: "STATS"
    });
    statsButton.clicked.connect(function() {
        Menu.triggerOption("Stats");
    });
}

init();

}()); // END LOCAL_SCOPE
