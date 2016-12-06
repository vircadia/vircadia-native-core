//
//  marketplacesDirectory.js
//
//  Created by David Rowe on 12 Nov 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Injected into marketplaces directory page.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Only used in WebTablet.

$(document).ready(function () {

    // Remove e-mail hyperlink.
    var letUsKnow = $("#letUsKnow");
    letUsKnow.replaceWith(letUsKnow.html());

});
