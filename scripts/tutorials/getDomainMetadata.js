//
//  Created by Zach Pomerantz on June 16, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var SERVER = 'https://metaverse.highfidelity.com/api/v1';

var OVERLAY = null;

// Every time you enter a domain, display the domain's metadata
location.hostChanged.connect(function(host) {
    print('Detected host change:', host);

    // Fetch the domain ID from the metaverse
    var placeData = request(SERVER + '/places/' + host);
    if (!placeData) {
        print('Cannot find place name - abandoning metadata request for', host);
        return;
        
    }
    var domainID = placeData.data.place.domain.id;
    print('Domain ID:', domainID);

    // Fetch the domain metadata from the metaverse
    var domainData = request(SERVER + '/domains/' + domainID);
    print(SERVER + '/domains/' + domainID);
    if (!domainData) {
        print('Cannot find domain data - abandoning metadata request for', domainID);
        return;
    }
    var metadata = domainData.domain;
    print('Domain metadata:', JSON.stringify(metadata));

    // Display the fetched metadata in an overlay
    displayMetadata(host, metadata);
});

Script.scriptEnding.connect(clearMetadata);

function displayMetadata(place, metadata) {
    clearMetadata();

    var COLOR_TEXT = { red: 255, green: 255, blue: 255 };
    var COLOR_BACKGROUND = { red: 0, green: 0, blue: 0 };
    var MARGIN = 200;
    var STARTING_OPACITY = 0.8;
    var FADE_AFTER_SEC = 2;
    var FADE_FOR_SEC = 4;

    var fade_per_sec = STARTING_OPACITY / FADE_FOR_SEC;
    var properties = {
        color: COLOR_TEXT,
        alpha: STARTING_OPACITY,
        backgroundColor: COLOR_BACKGROUND,
        backgroundAlpha: STARTING_OPACITY,
        font: { size: 24 },
        x: MARGIN,
        y: MARGIN
    };

    // Center the overlay on the screen
    properties.width = Window.innerWidth - MARGIN*2;
    properties.height = Window.innerHeight - MARGIN*2;

    // Parse the metadata into text
    parsed = [ 'Welcome to ' + place + '!',, ];
    if (metadata.description) {
        parsed.push(description);
    }
    if (metadata.tags && metadata.tags.length) {
        parsed.push('Tags: ' + metadata.tags.join(','));
    }
    if (metadata.capacity) {
        parsed.push('Capacity (max users): ' + metadata.capacity);
    }
    if (metadata.maturity) {
        parsed.push('Maturity: ' + metadata.maturity);
    }
    if (metadata.hosts && metadata.hosts.length) {
        parsed.push('Hosts: ' + metadata.tags.join(','));
    }
    if (metadata.online_users) {
        parsed.push('Users online: ' + metadata.online_users);
    }

    properties.text = parsed.join('\n\n');

    // Display the overlay
    OVERLAY = Overlays.addOverlay('text', properties);

    // Fade out the overlay over 10 seconds
    !function() {
        var overlay = OVERLAY;
        var alpha = STARTING_OPACITY;

        var fade = function() {
            // Only fade so long as the same overlay is up
            if (overlay == OVERLAY) {
                alpha -= fade_per_sec / 10;
                if (alpha <= 0) {
                    clearMetadata();
                } else {
                    Overlays.editOverlay(overlay, { alpha: alpha, backgroundAlpha: alpha });
                    Script.setTimeout(fade, 100);
                }
            }
        };
        Script.setTimeout(fade, FADE_AFTER_SEC * 1000);
    }();
}

function clearMetadata() {
    if (OVERLAY) {
        Overlays.deleteOverlay(OVERLAY);
    }
}

// Request JSON from a url, synchronously
function request(url) {
    var req = new XMLHttpRequest();
    req.responseType = 'json';
    req.open('GET', url, false);
    req.send();
    return req.status == 200 ? req.response : null;
}

