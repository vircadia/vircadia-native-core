//
//  Created by Zach Pomerantz on June 16, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Avoid polluting the namespace
// !function() {
    var MetadataNotification = null;

    // Every time you enter a domain, display the domain's metadata
    SomeEvent.connect(function() {
        // Fetch the domain metadata from the directory

        // Display the fetched metadata in an overlay

    });

    // Remove the overlay if the script is stopped
    Script.scriptEnding.connect(teardown);

    function teardown() {
        // Close the overlay, if it exists

    }
// }();
