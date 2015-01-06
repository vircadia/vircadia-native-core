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

// Get and log the current downloads info ...

var downloadInfo,
    downloadInfoString;

downloadInfo = GlobalServices.getDownloadInfo();
downloadInfoString = "Downloads: ";
if (downloadInfo.downloading.length > 0) {
    downloadInfoString += downloadInfo.downloading.join("% ") + "% ";
}
downloadInfoString += "(" + downloadInfo.pending.toFixed(0) + " pending)";
print(downloadInfoString);
