//
//  overlaysExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/14/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Overlays class
//
//

    /*
    _testOverlayA.init(_glWidget, QString("https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg"), 
            QRect(100,100,62,40), QRect(0,0,62,40));
    xColor red = { 255, 0, 0 };
    _testOverlayA.setBackgroundColor(red);
    _testOverlayB.init(_glWidget, QString("https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg"), 
            QRect(170,100,62,40), QRect(0,80,62,40));
    */

var toolA = Overlays.addOverlay({
                    x: 100,
                    y: 100,
                    width: 62,
                    height: 40,
                    subImage: { x: 0, y: 0, width: 62, height: 40 },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    backgroundColor: { red: 255, green: 0, blue: 255},
                    alpha: 1.0
                });

function scriptEnding() {
    Overlays.deleteOverlay(toolA);
}
Script.scriptEnding.connect(scriptEnding);
