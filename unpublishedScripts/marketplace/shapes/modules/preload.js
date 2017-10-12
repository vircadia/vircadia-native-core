//
//  preload.js
//
//  Created by David Rowe on 11 Oct 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Preload:true, App */

Preload = (function () {
    // Provide facility to preload asset files so that they are in disk cache.
    // Global object.

    "use strict";

    var loadTimer = null,
        urlsToLoad = [],
        nextURLToLoad = 0,
        LOAD_INTERVAL = 50, // ms
        overlays = [],
        deleteTimer = null,
        DELETE_INTERVAL = LOAD_INTERVAL; // ms

    function findURLs(items) {
        var urls = [],
            i,
            length;

        function findURLsInObject(item) {
            var property;

            for (property in item) {
                if (item.hasOwnProperty(property)) {
                    if (property === "url" || property === "imageURL" || property === "imageOverlayURL") {
                        if (item[property]) {
                            urls.push(item[property]);
                        }
                    } else if (typeof item[property] === "object") {
                        findURLsInObject(item[property]);
                    }
                }
            }
        }

        for (i = 0, length = items.length; i < length; i++) {
            switch (typeof items[i]) {
                case "string":
                    urls.push(items[i]);
                    break;
                case "object":
                    findURLsInObject(items[i]);
                    break;
                default:
                    App.log("ERROR: Cannot find URL in item type " + (typeof items[i]));
            }
        }

        return urls;
    }

    function deleteOverlay() {
        if (overlays.length === 0) { // Just in case.
            deleteTimer = null;
            return;
        }

        Overlays.deleteOverlay(overlays[0]);
        overlays.shift();

        if (overlays.length > 0) {
            deleteTimer = Script.setTimeout(deleteOverlay, DELETE_INTERVAL);
        } else {
            deleteTimer = null;
        }
    }

    function loadNextURL() {

        function loadURL(url) {
            var DOMAIN_CORNER = { x: -16382, y: -16382, z: -16382 }, // Near but not right on domain corner.
                DUMMY_OVERLAY_PROPERTIES = {
                    fbx: {
                        overlay: "model",
                        dimensions: { x: 0.001, y: 0.001, z: 0.001 },
                        position: DOMAIN_CORNER,
                        ignoreRayIntersection: false,
                        alpha: 0.0,
                        visible: false
                    },
                    svg: {
                        overlay: "image3d",
                        scale: 0.001,
                        position: DOMAIN_CORNER,
                        ignoreRayIntersection: true,
                        alpha: 0.0,
                        visible: false
                    },
                    png: {
                        overlay: "image3d",
                        scale: 0.001,
                        position: DOMAIN_CORNER,
                        ignoreRayIntersection: true,
                        alpha: 0.0,
                        visible: false
                    }
                },
                fileType,
                properties;

            fileType = url.slice(-3);
            if (DUMMY_OVERLAY_PROPERTIES.hasOwnProperty(fileType)) {
                properties = Object.clone(DUMMY_OVERLAY_PROPERTIES[fileType]);
                properties.url = url;
                overlays.push(Overlays.addOverlay(properties.overlay, properties));
                if (deleteTimer === null) {
                    // Can't delete overlay straight away otherwise asset load is abandoned.
                    deleteTimer = Script.setTimeout(deleteOverlay, DELETE_INTERVAL);
                }
            } else {
                App.log("ERROR: Cannot preload asset " + url);
            }
        }

        // Find next URL that hasn't already been loaded;
        while (nextURLToLoad < urlsToLoad.length && urlsToLoad.indexOf(urlsToLoad[nextURLToLoad]) < nextURLToLoad) {
            nextURLToLoad += 1;
        }

        // Load the URL if there's one to load.
        if (nextURLToLoad < urlsToLoad.length) {
            // Load the URL.
            loadURL(urlsToLoad[nextURLToLoad]);
            nextURLToLoad += 1;

            // Load the next, if any, after a short delay.
            loadTimer = Script.setTimeout(loadNextURL, LOAD_INTERVAL);
        } else {
            loadTimer = null;
        }
    }

    function load(urls) {
        urlsToLoad = urlsToLoad.concat(urls);
        if (loadTimer === null) {
            loadNextURL();
        }
    }

    function tearDown() {
        var i,
            length;

        if (loadTimer) {
            Script.clearTimeout(loadTimer);
        }

        if (deleteTimer) {
            Script.clearTimeout(deleteTimer);
            for (i = 0, length = overlays.length; i < length; i++) {
                Overlays.deleteOverlay(overlays[i]);
            }
        }
    }

    Script.scriptEnding.connect(tearDown);

    return {
        findURLs: findURLs,
        load: load
    };

}());
