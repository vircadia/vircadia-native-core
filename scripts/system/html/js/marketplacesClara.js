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

(function () {
    // Can't use $(document).ready() because jQuery isn't loaded early enough by Clara Web page.

    var locationHref = "";
    var checkLocationInterval = undefined;

    function checkLocation() {
        // Have to manually monitor location for changes because Clara Web page replaced content rather than loading new page.

        if (location.href !== locationHref) {

            // Clara library page.
            if (location.href.indexOf("clara.io/library") !== -1) {
                var elements = $("a.thumbnail");
                for (var i = 0, length = elements.length; i < length; i++) {
                    var value = elements[i].getAttribute("href");
                    if (value.slice(-6) !== "/image") {
                        elements[i].setAttribute("href", value + "/image");
                    }
                }
            }

            // Clara item page.
            if (location.href.indexOf("clara.io/view/") !== -1) {
                var element = $("a[href^=\'/library\']")[0];
                var parameters = "?gameCheck=true&public=true";
                var href = element.getAttribute("href");
                if (href.slice(-parameters.length) !== parameters) {
                    element.setAttribute("href", href + parameters);
                }
                var buttons = $("a.embed-button").parent("div");
                if (buttons.length > 0) {
                    var downloadFBX = buttons.find("a[data-extension=\'fbx\']")[0];
                    downloadFBX.addEventListener("click", startAutoDownload);
                    var firstButton = buttons.children(":first-child")[0];
                    buttons[0].insertBefore(downloadFBX, firstButton);
                    downloadFBX.setAttribute("class", "btn btn-primary download");
                    downloadFBX.innerHTML = "<i class=\'glyphicon glyphicon-download-alt\'></i> Download to High Fidelity";
                    buttons.children(":nth-child(2), .btn-group , .embed-button").each(function () { this.remove(); });
                }
                var downloadTimer;
                function startAutoDownload() {
                    if (!downloadTimer) {
                        downloadTimer = setInterval(autoDownload, 1000);
                    }
                }
                function autoDownload() {
                    if ($("div.download-body").length !== 0) {
                        var downloadButton = $("div.download-body a.download-file");
                        if (downloadButton.length > 0) {
                            clearInterval(downloadTimer);
                            downloadTimer = null;
                            var href = downloadButton[0].href;
                            EventBridge.emitWebEvent("CLARA.IO DOWNLOAD " + href);
                            console.log("Clara.io FBX file download initiated for " + href);
                            $("a.btn.cancel").click();
                        };
                    } else {
                        clearInterval(downloadTimer);
                        downloadTimer = null;
                    }
                }
            }

            locationHref = location.href;
        }
    }

    function onLoad() {

        // Supporting styles from marketplaces.css.
        // Glyph font family, size, and spacing adjusted because HiFi-Glyphs cannot be used cross-domain.
        $("head").append(
            '<style>' +
                '#marketplace-navigation { font-family: Arial, Helvetica, sans-serif; width: 100%; height: 50px; background: #00b4ef; position: fixed; bottom: 0; }' +
                '#marketplace-navigation .glyph { margin-left: 20px; margin-right: 3px; font-family: sans-serif; color: #fff; font-size: 24px; line-height: 50px; }' +
                '#marketplace-navigation .text { color: #fff; font-size: 18px; line-height: 50px; vertical-align: top; position: relative; top: 1px; }' +
                '#marketplace-navigation input { position: absolute; right: 20px; margin-top: 12px; padding-left: 10px; padding-right: 10px; }' +
            '</style>'
        );

        // Supporting styles from edit-style.css.
        // Font family, size, and position adjusted because Raleway-Bold cannot be used cross-domain.
        $("head").append(
            '<style>' +
                'input[type=button] { font-family: Arial, Helvetica, sans-serif; font-weight: bold; font-size: 12px; text-transform: uppercase; vertical-align: center; height: 28px; min-width: 100px; padding: 0 15px; border-radius: 5px; border: none; color: #fff; background-color: #000; background: linear-gradient(#343434 20%, #000 100%); cursor: pointer; }' +
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
            '<span class="glyph">&#x1f6c8;</span> <span class="text">Check out other marketplaces.</span>' +
                '<input id="all-markets" type="button" class="white" value="See All Markets" />' +
            '</div>'
        );

        // Marketplace footer action.
        $("#all-markets").on("click", function () {
            $("#marketplace-content").attr("src", "marketplacesDirectory.html");
            EventBridge.emitWebEvent("RELOAD_DIRECTORY");
        });

        checkLocation();
        checkLocationInterval = setInterval(checkLocation, 1000);
    }

    function onUnload() {
        clearInterval(checkLocationInterval);
        checkLocationInterval = undefined;
        locationHref = "";
    }

    window.addEventListener("load", onLoad);
    window.addEventListener("unload", onUnload);

}());
