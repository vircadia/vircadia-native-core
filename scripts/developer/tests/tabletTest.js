//
//  tabletTest.js
//
//  Created by Anthony J. Thibault on 2016-12-15
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Adds a BAM! button to the tablet ui.

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    text: "BAM!!!"
});

// change the name and isActive state every second...
var names = ["BAM!", "BAM!!", "BAM!!!", "BAM!!!!"];
var nameIndex = 0;
Script.setInterval(function () {
    nameIndex = (nameIndex + 1) % names.length;
    button.editProperties({
        isActive: (nameIndex & 0x1) == 0,
        text: names[nameIndex]
    });
}, 1000);

button.clicked.connect(function () {
    print("AJT: BAM!!! CLICK from JS!");
    var url = "https://news.ycombinator.com/";
    tablet.gotoWebScreen(url);
});

Script.scriptEnding.connect(function () {
    tablet.removeButton(button);
});
