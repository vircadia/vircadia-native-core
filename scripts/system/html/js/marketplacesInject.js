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
    var isPreparing = false;  // Explicitly track download request status.

    var commerceMode = false;
    var userIsLoggedIn = false;
    var walletNeedsSetup = false;
    var metaverseServerURL = "https://metaverse.highfidelity.com";

    function injectCommonCode(isDirectoryPage) {

        // Supporting styles from marketplaces.css.
        // Glyph font family, size, and spacing adjusted because HiFi-Glyphs cannot be used cross-domain.
        $("head").append(
            '<style>' +
                '#marketplace-navigation { font-family: Arial, Helvetica, sans-serif; width: 100%; height: 50px; background: #00b4ef; position: fixed; bottom: 0; z-index: 1000; }' +
                '#marketplace-navigation .glyph { margin-left: 10px; margin-right: 3px; font-family: sans-serif; color: #fff; font-size: 24px; line-height: 50px; }' +
                '#marketplace-navigation .text { color: #fff; font-size: 16px; line-height: 50px; vertical-align: top; position: relative; top: 1px; }' +
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
        var isInitialHiFiPage = location.href === metaverseServerURL + "/marketplace?";
        $("body").append(
            '<div id="marketplace-navigation">' +
                (!isInitialHiFiPage ? '<input id="back-button" type="button" class="white" value="&lt; Back" />' : '') +
                (isInitialHiFiPage ? '<span class="glyph">&#x1f6c8;</span> <span class="text">Get items from Clara.io!</span>' : '') +
                (!isDirectoryPage ? '<input id="all-markets" type="button" class="white" value="See All Markets" />' : '') +
                (isDirectoryPage ? '<span class="right"><span class="glyph">&#x1f6c8;</span> <span class="text">Select a marketplace to explore.</span><span>' : '') +
            '</div>'
        );

        // Footer actions.
        $("#back-button").on("click", function () {
            (document.referrer !== "") ? window.history.back() : window.location = (metaverseServerURL + "/marketplace?");
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
            window.location = "https://clara.io/library?gameCheck=true&public=true";
        });
        $('#exploreHifiMarketplace').on('click', function () {
            window.location = "http://www.highfidelity.com/marketplace";
        });
    }

    emitWalletSetupEvent = function() {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "WALLET_SETUP"
        }));
    }

    function maybeAddSetupWalletButton() {
        if (!$('body').hasClass("walletsetup-injected") && userIsLoggedIn && walletNeedsSetup) {
            $('body').addClass("walletsetup-injected");

            var resultsElement = document.getElementById('results');
            var setupWalletElement = document.createElement('div');
            setupWalletElement.classList.add("row");
            setupWalletElement.id = "setupWalletDiv";
            setupWalletElement.style = "height:60px;margin:20px 10px 10px 10px;padding:12px 5px;" +
                "background-color:#D6F4D8;border-color:#aee9b2;border-width:2px;border-style:solid;border-radius:5px;";

            var span = document.createElement('span');
            span.style = "margin:10px 5px;color:#1b6420;font-size:15px;";
            span.innerHTML = "<a href='#' onclick='emitWalletSetupEvent(); return false;'>Setup your Wallet</a> to get money and shop in Marketplace.";

            var xButton = document.createElement('a');
            xButton.id = "xButton";
            xButton.setAttribute('href', "#");
            xButton.style = "width:50px;height:100%;margin:0;color:#ccc;font-size:20px;";
            xButton.innerHTML = "X";
            xButton.onclick = function () {
                setupWalletElement.remove();
                dummyRow.remove();
            };

            setupWalletElement.appendChild(span);
            setupWalletElement.appendChild(xButton);

            resultsElement.insertBefore(setupWalletElement, resultsElement.firstChild);

            // Dummy row for padding
            var dummyRow = document.createElement('div');
            dummyRow.classList.add("row");
            dummyRow.style = "height:15px;";
            resultsElement.insertBefore(dummyRow, resultsElement.firstChild);
        }
    }

    function maybeAddLogInButton() {
        if (!$('body').hasClass("login-injected") && !userIsLoggedIn) {
            $('body').addClass("login-injected");
            var resultsElement = document.getElementById('results');
            if (!resultsElement) { // If we're on the main page, this will evaluate to `true`
                resultsElement = document.getElementById('item-show');
                resultsElement.style = 'margin-top:0;';
            }
            var logInElement = document.createElement('div');
            logInElement.classList.add("row");
            logInElement.id = "logInDiv";
            logInElement.style = "height:60px;margin:20px 10px 10px 10px;padding:5px;" +
                "background-color:#D6F4D8;border-color:#aee9b2;border-width:2px;border-style:solid;border-radius:5px;";

            var button = document.createElement('a');
            button.classList.add("btn");
            button.classList.add("btn-default");
            button.id = "logInButton";
            button.setAttribute('href', "#");
            button.innerHTML = "LOG IN";
            button.style = "width:80px;height:100%;margin-top:0;margin-left:10px;padding:13px;font-weight:bold;background:linear-gradient(white, #ccc);";
            button.onclick = function () {
                EventBridge.emitWebEvent(JSON.stringify({
                    type: "LOGIN"
                }));
            };

            var span = document.createElement('span');
            span.style = "margin:10px;color:#1b6420;font-size:15px;";
            span.innerHTML = "to purchase items from the Marketplace.";

            var xButton = document.createElement('a');
            xButton.id = "xButton";
            xButton.setAttribute('href', "#");
            xButton.style = "width:50px;height:100%;margin:0;color:#ccc;font-size:20px;";
            xButton.innerHTML = "X";
            xButton.onclick = function () {
                logInElement.remove();
                dummyRow.remove();
            };

            logInElement.appendChild(button);
            logInElement.appendChild(span);
            logInElement.appendChild(xButton);

            resultsElement.insertBefore(logInElement, resultsElement.firstChild);

            // Dummy row for padding
            var dummyRow = document.createElement('div');
            dummyRow.classList.add("row");
            dummyRow.style = "height:15px;";
            resultsElement.insertBefore(dummyRow, resultsElement.firstChild);
        }
    }

    function maybeAddPurchasesButton() {
        if (userIsLoggedIn) {
            // Why isn't this an id?! This really shouldn't be a class on the website, but it is.
            var navbarBrandElement = document.getElementsByClassName('navbar-brand')[0];
            var purchasesElement = document.createElement('a');
            var dropDownElement = document.getElementById('user-dropdown');

            $('#user-dropdown').find('.username')[0].style = "max-width:80px;white-space:nowrap;overflow:hidden;" +
                "text-overflow:ellipsis;display:inline-block;position:relative;top:4px;";
            $('#user-dropdown').find('.caret')[0].style = "position:relative;top:-3px;";

            purchasesElement.id = "purchasesButton";
            purchasesElement.setAttribute('href', "#");
            purchasesElement.innerHTML = "My Purchases";
            // FRONTEND WEBDEV RANT: The username dropdown should REALLY not be programmed to be on the same
            //     line as the search bar, overlaid on top of the search bar, floated right, and then relatively bumped up using "top:-50px".
            purchasesElement.style = "height:100%;margin-top:18px;font-weight:bold;float:right;margin-right:" + (dropDownElement.offsetWidth + 30) +
                "px;position:relative;z-index:999;";
            navbarBrandElement.parentNode.insertAdjacentElement('beforeend', purchasesElement);
            $('#purchasesButton').on('click', function () {
                EventBridge.emitWebEvent(JSON.stringify({
                    type: "PURCHASES",
                    referrerURL: window.location.href
                }));
            });
        }
    }

    function changeDropdownMenu() {
        var logInOrOutButton = document.createElement('a');
        logInOrOutButton.id = "logInOrOutButton";
        logInOrOutButton.setAttribute('href', "#");
        logInOrOutButton.innerHTML = userIsLoggedIn ? "Log Out" : "Log In";
        logInOrOutButton.onclick = function () {
            EventBridge.emitWebEvent(JSON.stringify({
                type: "LOGIN"
            }));
        };

        $($('.dropdown-menu').find('li')[0]).append(logInOrOutButton);

        $('a[href="/marketplace?view=mine"]').each(function () {
            $(this).attr('href', '#');
            $(this).on('click', function () {
                EventBridge.emitWebEvent(JSON.stringify({
                    type: "MY_ITEMS"
                }));
            });
        });
    }

    function buyButtonClicked(id, name, author, price, href, referrer) {
        EventBridge.emitWebEvent(JSON.stringify({
            type: "CHECKOUT",
            itemId: id,
            itemName: name,
            itemPrice: price ? parseInt(price, 10) : 0,
            itemHref: href,
            referrer: referrer
        }));
    }

    function injectBuyButtonOnMainPage() {
        var cost;

        // Unbind original mouseenter and mouseleave behavior
        $('body').off('mouseenter', '#price-or-edit .price');
        $('body').off('mouseleave', '#price-or-edit .price');

        $('.grid-item').find('#price-or-edit').each(function () {
            $(this).css({ "margin-top": "0" });
        });

        $('.grid-item').find('#price-or-edit').find('a').each(function() {
            if ($(this).attr('href') !== '#') { // Guard necessary because of the AJAX nature of Marketplace site
                $(this).attr('data-href', $(this).attr('href'));
                $(this).attr('href', '#');
            }
            cost = $(this).closest('.col-xs-3').find('.item-cost').text();

            $(this).closest('.col-xs-3').prev().attr("class", 'col-xs-6');
            $(this).closest('.col-xs-3').attr("class", 'col-xs-6');

            var priceElement = $(this).find('.price')
            priceElement.css({
                "padding": "3px 5px",
                "height": "40px",
                "background": "linear-gradient(#00b4ef, #0093C5)",
                "color": "#FFF",
                "font-weight": "600",
                "line-height": "34px"
            });

            if (parseInt(cost) > 0) {
                priceElement.css({ "width": "auto" });
                priceElement.html('<span class="hifi-glyph hifi-glyph-hfc" style="filter:invert(1);background-size:20px;' +
                    'width:20px;height:20px;position:relative;top:5px;"></span> ' + cost);
                priceElement.css({ "min-width": priceElement.width() + 30 });
            }
        });

        // change pricing to GET/BUY on button hover
        $('body').on('mouseenter', '#price-or-edit .price', function () {
            var $this = $(this);
            $this.data('initialHtml', $this.html());

            var cost = $(this).parent().siblings().text();
            if (parseInt(cost) > 0) {
                $this.text('BUY');
            } else {
                $this.text('GET');
            }
        });

        $('body').on('mouseleave', '#price-or-edit .price', function () {
            var $this = $(this);
            $this.html($this.data('initialHtml'));
        });


        $('.grid-item').find('#price-or-edit').find('a').on('click', function () {
            buyButtonClicked($(this).closest('.grid-item').attr('data-item-id'),
                $(this).closest('.grid-item').find('.item-title').text(),
                $(this).closest('.grid-item').find('.creator').find('.value').text(),
                $(this).closest('.grid-item').find('.item-cost').text(),
                $(this).attr('data-href'),
                "mainPage");
        });
    }

    function injectUnfocusOnSearch() {
        // unfocus input field on search, thus hiding virtual keyboard
        $('#search-box').on('submit', function () {
            if (document.activeElement) {
                document.activeElement.blur();
            }
        });
    }

    // fix for 10108 - marketplace category cannot scroll
    function injectAddScrollbarToCategories() {
        $('#categories-dropdown').on('show.bs.dropdown', function () {
            $('body > div.container').css('display', 'none')
            $('#categories-dropdown > ul.dropdown-menu').css({ 'overflow': 'auto', 'height': 'calc(100vh - 110px)' })
        });

        $('#categories-dropdown').on('hide.bs.dropdown', function () {
            $('body > div.container').css('display', '')
            $('#categories-dropdown > ul.dropdown-menu').css({ 'overflow': '', 'height': '' })
        });
    }

    function injectHiFiCode() {
        if (commerceMode) {
            maybeAddLogInButton();
            maybeAddSetupWalletButton();

            if (!$('body').hasClass("code-injected")) {

                $('body').addClass("code-injected");
                changeDropdownMenu();

                var target = document.getElementById('templated-items');
                // MutationObserver is necessary because the DOM is populated after the page is loaded.
                // We're searching for changes to the element whose ID is '#templated-items' - this is
                //     the element that gets filled in by the AJAX.
                var observer = new MutationObserver(function (mutations) {
                    mutations.forEach(function (mutation) {
                        injectBuyButtonOnMainPage();
                    });
                    //observer.disconnect();
                });
                var config = { attributes: true, childList: true, characterData: true };
                observer.observe(target, config);

                // Try this here in case it works (it will if the user just pressed the "back" button,
                //     since that doesn't trigger another AJAX request.
                injectBuyButtonOnMainPage();
                maybeAddPurchasesButton();
            }
        }

        injectUnfocusOnSearch();
        injectAddScrollbarToCategories();
    }

    function injectHiFiItemPageCode() {
        if (commerceMode) {
            maybeAddLogInButton();

            if (!$('body').hasClass("code-injected")) {

                $('body').addClass("code-injected");
                changeDropdownMenu();

                var purchaseButton = $('#side-info').find('.btn').first();

                var href = purchaseButton.attr('href');
                purchaseButton.attr('href', '#');
                var availability = $.trim($('.item-availability').text());
                if (availability === 'available') {
                    purchaseButton.css({
                        "background": "linear-gradient(#00b4ef, #0093C5)",
                        "color": "#FFF",
                        "font-weight": "600",
                        "padding-bottom": "10px"
                    });
                } else {
                    purchaseButton.css({
                        "background": "linear-gradient(#a2a2a2, #fefefe)",
                        "color": "#000",
                        "font-weight": "600",
                        "padding-bottom": "10px"
                    });
                }

                var cost = $('.item-cost').text();
                if (availability !== 'available') {
                    purchaseButton.html('UNAVAILABLE (' + availability + ')');
                } else if (parseInt(cost) > 0 && $('#side-info').find('#buyItemButton').size() === 0) {
                    purchaseButton.html('PURCHASE <span class="hifi-glyph hifi-glyph-hfc" style="filter:invert(1);background-size:20px;' +
                        'width:20px;height:20px;position:relative;top:5px;"></span> ' + cost);
                }

                purchaseButton.on('click', function () {
                    if ('available' === availability) {
                        buyButtonClicked(window.location.pathname.split("/")[3],
                            $('#top-center').find('h1').text(),
                            $('#creator').find('.value').text(),
                            cost,
                            href,
                            "itemPage");
                        }
                    });
                maybeAddPurchasesButton();
            }
        }

        injectUnfocusOnSearch();
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

                // One file request at a time.
                if (isPreparing) {
                    console.log("WARNING: Clara.io FBX: Prepare only one download at a time");
                    return;
                }

                // User must be able to write to Asset Server.
                if (!canWriteAssets) {
                    console.log("ERROR: Clara.io FBX: File download cancelled because no permissions to write to Asset Server");
                    EventBridge.emitWebEvent(WARN_USER_NO_PERMISSIONS);
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
                // 13 Jan 2017: Specify FBX version 5 and remove some options in order to make Clara.io site more likely to
                // be successful in generating zip files.
                var XMLHTTPREQUEST_URL = "https://clara.io/api/scenes/{uuid}/export/fbx?fbxUnit=Meter&fbxVersion=5&fbxEmbedTextures=true&imageFormat=WebGL";

                var uuid = location.href.match(/\/view\/([a-z0-9\-]*)/)[1];
                var url = XMLHTTPREQUEST_URL.replace("{uuid}", uuid);

                xmlHttpRequest = new XMLHttpRequest();
                var responseTextIndex = 0;
                var zipFileURL = "";

                xmlHttpRequest.onreadystatechange = function () {
                    // Messages are appended to responseText; process the new ones.
                    var message = this.responseText.slice(responseTextIndex);
                    var statusMessage = "";

                    if (isPreparing) {  // Ignore messages in flight after finished/cancelled.
                        var lines = message.split(/[\n\r]+/);

                        for (var i = 0, length = lines.length; i < length; i++) {
                            if (lines[i].slice(0, 5) === "data:") {
                                // Parse line.
                                var data;
                                try {
                                    data = JSON.parse(lines[i].slice(5));
                                }
                                catch (e) {
                                    data = {};
                                }

                                // Extract status message.
                                if (data.hasOwnProperty("message") && data.message !== null) {
                                    statusMessage = data.message;
                                    console.log("Clara.io FBX: " + statusMessage);
                                }

                                // Extract zip file URL.
                                if (data.hasOwnProperty("files") && data.files.length > 0) {
                                    zipFileURL = data.files[0].url;
                                    if (zipFileURL.slice(-4) !== ".zip") {
                                        console.log(JSON.stringify(data));  // Data for debugging.
                                    }
                                }
                            }
                        }

                        if (statusMessage !== "") {
                            // Update the UI with the most recent status message.
                            EventBridge.emitWebEvent(CLARA_IO_STATUS + " " + statusMessage);
                        }
                    }

                    responseTextIndex = this.responseText.length;
                };

                // Note: onprogress doesn't have computable total length so can't use it to determine % complete.

                xmlHttpRequest.onload = function () {
                    var statusMessage = "";

                    if (!isPreparing) {
                        return;
                    }

                    isPreparing = false;

                    var HTTP_OK = 200;
                    if (this.status !== HTTP_OK) {
                        statusMessage = "Zip file request terminated with " + this.status + " " + this.statusText;
                        console.log("ERROR: Clara.io FBX: " + statusMessage);
                        EventBridge.emitWebEvent(CLARA_IO_STATUS + " " + statusMessage);
                    } else if (zipFileURL.slice(-4) !== ".zip") {
                        statusMessage = "Error creating zip file for download.";
                        console.log("ERROR: Clara.io FBX: " + statusMessage + ": " + zipFileURL);
                        EventBridge.emitWebEvent(CLARA_IO_STATUS + " " + statusMessage);
                    } else {
                        EventBridge.emitWebEvent(CLARA_IO_DOWNLOAD + " " + zipFileURL);
                        console.log("Clara.io FBX: File download initiated for " + zipFileURL);
                    }

                    xmlHttpRequest = null;
                }

                isPreparing = true;

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
        isPreparing = false;

        if (xmlHttpRequest) {
            xmlHttpRequest.abort();
            xmlHttpRequest = null;
            console.log("Clara.io FBX: File download cancelled");
            EventBridge.emitWebEvent(CLARA_IO_CANCELLED_DOWNLOAD);
        }
    }

    function injectCode() {
        var DIRECTORY = 0;
        var HIFI = 1;
        var CLARA = 2;
        var HIFI_ITEM_PAGE = 3;
        var pageType = DIRECTORY;

        if (location.href.indexOf("highfidelity.com/") !== -1) { pageType = HIFI; }
        if (location.href.indexOf("clara.io/") !== -1) { pageType = CLARA; }
        if (location.href.indexOf("highfidelity.com/marketplace/items/") !== -1) { pageType = HIFI_ITEM_PAGE; }

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
            case HIFI_ITEM_PAGE:
                injectHiFiItemPageCode();
                break;

        }
    }

    function onLoad() {
        EventBridge.scriptEventReceived.connect(function (message) {
            if (message.slice(0, CAN_WRITE_ASSETS.length) === CAN_WRITE_ASSETS) {
                canWriteAssets = message.slice(-4) === "true";
            } else if (message.slice(0, CLARA_IO_CANCEL_DOWNLOAD.length) === CLARA_IO_CANCEL_DOWNLOAD) {
                cancelClaraDownload();
            } else {
                var parsedJsonMessage = JSON.parse(message);

                if (parsedJsonMessage.type === "marketplaces") {
                    if (parsedJsonMessage.action === "commerceSetting") {
                        commerceMode = !!parsedJsonMessage.data.commerceMode;
                        userIsLoggedIn = !!parsedJsonMessage.data.userIsLoggedIn;
                        walletNeedsSetup = !!parsedJsonMessage.data.walletNeedsSetup;
                        metaverseServerURL = parsedJsonMessage.data.metaverseServerURL;
                        injectCode();
                    }
                }
            }
        });

        // Request commerce setting
        // Code is injected into the webpage after the setting comes back.
        EventBridge.emitWebEvent(JSON.stringify({
            type: "REQUEST_SETTING"
        }));
    }

    // Load / unload.
    window.addEventListener("load", onLoad);  // More robust to Web site issues than using $(document).ready().
    window.addEventListener("page:change", onLoad);  // Triggered after Marketplace HTML is changed
}());
