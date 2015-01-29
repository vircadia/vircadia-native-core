//
//  progress.js
//  examples
//
//  Created by David Rowe on 29 Jan 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script displays a progress download indicator when downloads are in progress.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var progress = 100;  // %

    function updateInfo(info) {
        var i;

        if (info.downloading.length + info.pending === 0) {
            progress = 100;
        } else {
            progress = 0;
            for (i = 0; i < info.downloading.length; i += 1) {
                progress += info.downloading[i];
            }
            progress = progress / (info.downloading.length + info.pending);
        }

        print(progress.toFixed(0) + "%");
    }

    GlobalServices.downloadInfoChanged.connect(updateInfo);
    GlobalServices.updateDownloadInfo();
}());
