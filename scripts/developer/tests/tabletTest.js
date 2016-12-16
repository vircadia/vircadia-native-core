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
    icon: "https://s3.amazonaws.com/hifi-public/tony/icons/hat-up.svg",
    color: "#ff6f6f",
    text: "BAM!!!"
});

// change the color and name every second...
var colors = ["#ff6f6f", "#6fff6f", "#6f6fff"];
var names = ["BAM!", "BAM!!", "BAM!!!"];
var colorIndex = 0;
Script.setInterval(function () {
    colorIndex = (colorIndex + 1) % colors.length;
    button.editProperties({
        color: colors[colorIndex],
        text: names[colorIndex]
    });
}, 1000);

button.clicked.connect(function () {
    print("AJT: BAMM!!! CLICK from JS!");
});

Script.scriptEnding.connect(function () {
    tablet.removeButton(button);
});
