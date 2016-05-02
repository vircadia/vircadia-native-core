//
//  downloadInfoExample.js
//  examples/example
//
//  Created by David Rowe on 5 Jan 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Display downloads information the same as in the stats.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var downloadInfo,
    downloadInfoOverlay;

function formatInfo(info) {
    var string = "Downloads: ",
        i;

    for (i = 0; i < info.downloading.length; i += 1) {
        string += info.downloading[i].toFixed(0) + "% ";
    }

    string += "(" + info.pending.toFixed(0) + " pending)";

    return string;
}


// Get and log the current downloads info ...

downloadInfo = GlobalServices.getDownloadInfo();
print(formatInfo(downloadInfo));


// Display and update the downloads info in an overlay ...

function setUp() {
    downloadInfoOverlay = Overlays.addOverlay("text", {
        x: 300,
        y: 200,
        width: 300,
        height: 50,
        color: { red: 255, green: 255, blue: 255 },
        alpha: 1.0,
        backgroundColor: { red: 127, green: 127, blue: 127 },
        backgroundAlpha: 0.5,
        topMargin: 15,
        leftMargin: 20,
        text: ""
    });
}

function updateInfo(info) {
    Overlays.editOverlay(downloadInfoOverlay, { text: formatInfo(info) });
}

function tearDown() {
    Overlays.deleteOverlay(downloadInfoOverlay);
}

setUp();
GlobalServices.downloadInfoChanged.connect(updateInfo);
GlobalServices.updateDownloadInfo();
Script.scriptEnding.connect(tearDown);
