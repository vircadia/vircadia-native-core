//
//  marketplacesClara.js
//
//  Created by David Rowe on 12 Nov 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Injected into Clara.io marketplace Web page.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function onLoad() {
    // Supporting styles from marketplaces.css.
    $("head").append(
        '<style>' +
            '#marketplace-navigation { width: 100%; height: 50px; background: #00b4ef; position: fixed; bottom: 0; }' +
            '#marketplace-navigation .glyph { margin-left: 20px; font-family: HiFi-Glyphs; color: #fff; font-size: 40px; line-height: 50px; }' +
            '#marketplace-navigation .text { color: #fff; font-size: 18px; line-height: 50px; vertical-align: top; position: relative; top: 1px; }' +
            '#marketplace-navigation input { float: right; margin-right: 50px; margin-top: 12px; }' +
        '</style>'
    );

    // Supporting styles from edit-style.css.
    $("head").append(
        '<style>' +
            'input[type=button] { font-family: Raleway-Bold; font-size: 13px; text-transform: uppercase; vertical-align: top; height: 28px; min-width: 120px; padding: 0px 18px; margin-right: 6px; border-radius: 5px; border: none; color: #fff; background-color: #000; background: linear-gradient(#343434 20%, #000 100%); cursor: pointer; }' +
            'input[type=button].white { color: #121212; background-color: #afafaf; background: linear-gradient(#fff 20%, #afafaf 100%); }' +
            'input[type=button].white:enabled:hover { background: linear-gradient(#fff, #fff); border: none; }' +
            'input[type=button].white:active { background: linear-gradient(#afafaf, #afafaf); }' +
        '</style>'
    );

    // Make space for marketplaces footer.
    $("head").append(
        '<style>' +
            '#app { margin-bottom: 135px; }' +
            '.footer { bottom: 50px; }' +
        '</style>'
    );

    // Marketplaces footer.
    $("body").append(
        '<div id="marketplace-navigation">' +
            '<span class="glyph">[</span> <span class="text">Check out other marketplaces.</span>' +
            '<input id="all-markets" type="button" class="white" value="See All Markets" />' +
        '</div>'
    );

    // Marketplace footer action.
    $("#all-markets").on("click", function () {
        $("#marketplace-content").attr("src", "marketplacesDirectory.html");
        EventBridge.emitWebEvent("RELOAD_DIRECTORY");
    });
}

window.addEventListener("load", onLoad);
