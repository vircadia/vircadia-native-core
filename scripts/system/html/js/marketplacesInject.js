//
//  marketplacesInject.js
//
//  Created by David Rowe on 12 Nov 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Injected into marketplace Web pages.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    // Event bridge messages.
    var CLARA_IO_DOWNLOAD = "CLARA.IO DOWNLOAD";
    var CLARA_IO_STATUS = "CLARA.IO STATUS";
    var CLARA_IO_CANCEL_DOWNLOAD = "CLARA.IO CANCEL DOWNLOAD";
    var CLARA_IO_CANCELLED_DOWNLOAD = "CLARA.IO CANCELLED DOWNLOAD";
    var GOTO_DIRECTORY = "GOTO_DIRECTORY";
    var QUERY_CAN_WRITE_ASSETS = "QUERY_CAN_WRITE_ASSETS";
    var CAN_WRITE_ASSETS = "CAN_WRITE_ASSETS";
    var WARN_USER_NO_PERMISSIONS = "WARN_USER_NO_PERMISSIONS";

    var canWriteAssets = false;
    var xmlHttpRequest = null;

    function injectCommonCode(isDirectoryPage) {

        // Supporting styles from marketplaces.css.
        // Glyph font family, size, and spacing adjusted because HiFi-Glyphs cannot be used cross-domain.
        $("head").append(
            '<style>' +
                '#marketplace-navigation { font-family: Arial, Helvetica, sans-serif; width: 100%; height: 50px; background: #00b4ef; position: fixed; bottom: 0; z-index: 1000; }' +
                '#marketplace-navigation .glyph { margin-left: 20px; margin-right: 3px; font-family: sans-serif; color: #fff; font-size: 24px; line-height: 50px; }' +
                '#marketplace-navigation .text { color: #fff; font-size: 18px; line-height: 50px; vertical-align: top; position: relative; top: 1px; }' +
                '#marketplace-navigation input#back-button { position: absolute; left: 20px; margin-top: 12px; padding-left: 0; padding-right: 5px; }' +
                '#marketplace-navigation input#all-markets { position: absolute; right: 20px; margin-top: 12px; padding-left: 15px; padding-right: 15px; }' +
                '#marketplace-navigation .right { position: absolute; right: 20px; }' +
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

        // Footer.
        var isInitialHiFiPage = location.href === "https://metaverse.highfidelity.com/marketplace?";
        $("body").append(
            '<div id="marketplace-navigation">' +
                (!isInitialHiFiPage ? '<input id="back-button" type="button" class="white" value="&lt; Back" />' : '') +
                (isInitialHiFiPage ? '<span class="glyph">&#x1f6c8;</span> <span class="text">See also other marketplaces.</span>' : '') +
                (!isDirectoryPage ? '<input id="all-markets" type="button" class="white" value="See All Markets" />' : '') +
                (isDirectoryPage ? '<span class="right"><span class="glyph">&#x1f6c8;</span> <span class="text">Select a marketplace to explore.</span><span>' : '') +
            '</div>'
        );

        // Footer actions.
        $("#back-button").on("click", function () {
            window.history.back();
        });
        $("#all-markets").on("click", function () {
            EventBridge.emitWebEvent(GOTO_DIRECTORY);
        });
    }

    function injectDirectoryCode() {

        // Remove e-mail hyperlink.
        var letUsKnow = $("#letUsKnow");
        letUsKnow.replaceWith(letUsKnow.html());

        // Add button links.
        $('#exploreClaraMarketplace').on('click', function () {
            window.location = "https://clara.io/library?gameCheck=true&public=true"
        });
        $('#exploreHifiMarketplace').on('click', function () {
            window.location = "http://www.highfidelity.com/marketplace"
        });
    }

    function injectHiFiCode() {
        // Nothing to do.
    }

    function updateClaraCode() {
        // Have to repeatedly update Clara page because its content can change dynamically without location.href changing.

        // Clara library page.
        if (location.href.indexOf("clara.io/library") !== -1) {
            // Make entries navigate to "Image" view instead of default "Real Time" view.
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
            // Make site navigation links retain gameCheck etc. parameters.
            var element = $("a[href^=\'/library\']")[0];
            var parameters = "?gameCheck=true&public=true";
            var href = element.getAttribute("href");
            if (href.slice(-parameters.length) !== parameters) {
                element.setAttribute("href", href + parameters);
            }

            // Remove unwanted buttons and replace download options with a single "Download to High Fidelity" button.
            var buttons = $("a.embed-button").parent("div");
            var downloadFBX;
            if (buttons.find("div.btn-group").length > 0) {
                buttons.children(".btn-primary, .btn-group , .embed-button").each(function () { this.remove(); });
                if ($("#hifi-download-container").length === 0) {  // Button hasn't been moved already.
                    downloadFBX = $('<a class="btn btn-primary"><i class=\'glyphicon glyphicon-download-alt\'></i> Download to High Fidelity</a>');
                    buttons.prepend(downloadFBX);
                    downloadFBX[0].addEventListener("click", startAutoDownload);
                }
            }

            // Move the "Download to High Fidelity" button to be more visible on tablet.
            if ($("#hifi-download-container").length === 0 && window.innerWidth < 700) {
                var downloadContainer = $('<div id="hifi-download-container"></div>');
                $(".top-title .col-sm-4").append(downloadContainer);
                downloadContainer.append(downloadFBX);
            }

            // Automatic download to High Fidelity.
            function startAutoDownload() {

                // User must be able to write to Asset Server.
                if (!canWriteAssets) {
                    console.log("ERROR: Clara.io FBX: File download cancelled because no permissions to write to Asset Server");
                    EventBridge.emitWebEvent(WARN_USER_NO_PERMISSIONS);
                    event.stopPropagation();
                    return;
                }

                // User must be logged in.
                var loginButton = $("#topnav a[href='/signup']");
                if (loginButton.length > 0) {
                    loginButton[0].click();
                    return;
                }

                // Obtain zip file to download for requested asset.
                // Reference: https://clara.io/learn/sdk/api/export

                //var XMLHTTPREQUEST_URL = "https://clara.io/api/scenes/{uuid}/export/fbx?zip=true&centerScene=true&alignSceneGround=true&fbxUnit=Meter&fbxVersion=7&fbxEmbedTextures=true&imageFormat=WebGL";
                // 12 Jan 21017: Remove "&centerScene=true" option because it causes the Clara.io site to not generate zip files
                // for some models (e.g., "Julia" and "Ur Draug").
                var XMLHTTPREQUEST_URL = "https://clara.io/api/scenes/{uuid}/export/fbx?zip=true&alignSceneGround=true&fbxUnit=Meter&fbxVersion=7&fbxEmbedTextures=true&imageFormat=WebGL";

                var uuid = location.href.match(/\/view\/([a-z0-9\-]*)/)[1];
                var url = XMLHTTPREQUEST_URL.replace("{uuid}", uuid);

                xmlHttpRequest = new XMLHttpRequest();
                var responseTextIndex = 0;
                var statusMessage = "";
                var zipFileURL = "";

                xmlHttpRequest.onreadystatechange = function () {
                    // Messages are appended to responseText; process the new one.
                    var message = this.responseText.slice(responseTextIndex);

                    if (message.slice(0, 5) === "data:") {
                        var data = JSON.parse(message.slice(5));

                        // Extract status message.
                        if (data.hasOwnProperty("message") && data.message !== null) {
                            statusMessage = data.message;
                            console.log("Clara.io FBX: " + statusMessage);
                            EventBridge.emitWebEvent(CLARA_IO_STATUS + " " + statusMessage);
                        }

                        // Extract zip file URL.
                        if (data.hasOwnProperty("files") && data.files.length > 0) {
                            zipFileURL = data.files[0].url;
                            if (zipFileURL.slice(-4) !== ".zip") {
                                console.log(JSON.stringify(data));  // Data for debugging.
                            }
                        }
                    }

                    responseTextIndex = this.responseText.length;
                };

                // Note: onprogress doesn't have computable total length.

                xmlHttpRequest.onload = function () {
                    var HTTP_OK = 200;
                    if (this.status !== HTTP_OK) {
                        statusMessage = "Zip file request terminated with " + this.status + " " + this.statusText;
                        console.log("ERROR: Clara.io FBX: " + statusMessage);
                        EventBridge.emitWebEvent(CLARA_IO_STATUS + " " + statusMessage);
                        return;
                    }

                    if (zipFileURL.slice(-4) !== ".zip") {
                        statusMessage = "Error creating zip file for download";
                        console.log("ERROR: Clara.io FBX: " + statusMessage + ": " + zipFileURL);
                        EventBridge.emitWebEvent(CLARA_IO_STATUS + " " + statusMessage);
                        return;
                    }

                    EventBridge.emitWebEvent(CLARA_IO_DOWNLOAD + " " + zipFileURL);
                    console.log("Clara.io FBX: File download initiated for " + zipFileURL);

                    xmlHttpRequest = null;
                }

                console.log("Clara.io FBX: Request zip file for " + uuid);
                EventBridge.emitWebEvent(CLARA_IO_STATUS + " Initiating download");

                xmlHttpRequest.open("POST", url, true);
                xmlHttpRequest.setRequestHeader("Accept", "text/event-stream");
                xmlHttpRequest.send();
            }
        }
    }

    function injectClaraCode() {

        // Make space for marketplaces footer in Clara pages.
        $("head").append(
            '<style>' +
                '#app { margin-bottom: 135px; }' +
                '.footer { bottom: 50px; }' +
            '</style>'
        );

        // Condense space.
        $("head").append(
            '<style>' +
                'div.page-title { line-height: 1.2; font-size: 13px; }' +
                'div.page-title-row { padding-bottom: 0; }' +
            '</style>'
        );

        // Move "Download to High Fidelity" button.
        $("head").append(
            '<style>' +
                '#hifi-download-container { position: absolute; top: 6px; right: 16px; }' +
            '</style>'
        );

        // Update code injected per page displayed.
        var updateClaraCodeInterval = undefined;
        updateClaraCode();
        updateClaraCodeInterval = setInterval(function () {
            updateClaraCode();
        }, 1000);

        window.addEventListener("unload", function () {
            clearInterval(updateClaraCodeInterval);
            updateClaraCodeInterval = undefined;
        });

        EventBridge.emitWebEvent(QUERY_CAN_WRITE_ASSETS);
    }

    function cancelClaraDownload() {
        if (xmlHttpRequest) {
            xmlHttpRequest.abort();
            xmlHttpRequest = null;
            console.log("Clara.io FBX: File download cancelled");
            EventBridge.emitWebEvent(CLARA_IO_CANCELLED_DOWNLOAD);
        }
    }

    function onLoad() {

        EventBridge.scriptEventReceived.connect(function (message) {
            if (message.slice(0, CAN_WRITE_ASSETS.length) === CAN_WRITE_ASSETS) {
                canWriteAssets = message.slice(-4) === "true";
            }

            if (message.slice(0, CLARA_IO_CANCEL_DOWNLOAD.length) === CLARA_IO_CANCEL_DOWNLOAD) {
                cancelClaraDownload();
            }
        });

        var DIRECTORY = 0;
        var HIFI = 1;
        var CLARA = 2;
        var pageType = DIRECTORY;

        if (location.href.indexOf("highfidelity.com/") !== -1) { pageType = HIFI; }
        if (location.href.indexOf("clara.io/") !== -1) { pageType = CLARA; }

        injectCommonCode(pageType === DIRECTORY);
        switch (pageType) {
            case DIRECTORY:
                injectDirectoryCode();
                break;
            case HIFI:
                injectHiFiCode();
                break;
            case CLARA:
                injectClaraCode();
                break;
        }
    }

    // Load / unload.
    window.addEventListener("load", onLoad);  // More robust to Web site issues than using $(document).ready().

}());
