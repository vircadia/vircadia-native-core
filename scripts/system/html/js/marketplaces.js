//
//  marketplaces.js
//
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function loaded() {
    bindExploreButtons();
}

function bindExploreButtons() {
    $('#claraSignUp').on('click', function () {
        EventBridge.emitWebEvent("INJECT_CLARA");
    });
    $('#exploreClaraMarketplace').on('click', function () {
        EventBridge.emitWebEvent("INJECT_CLARA");
        window.location = "https://clara.io/library?gameCheck=true&public=true"
    });
    $('#exploreHifiMarketplace').on('click', function () {
        EventBridge.emitWebEvent("INJECT_HIFI");
        window.location = "http://www.highfidelity.com/marketplace"
    });
}
