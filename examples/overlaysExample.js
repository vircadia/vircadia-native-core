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

var swatch1 = Overlays.addOverlay({
                    x: 100,
                    y: 200,
                    width: 31,
                    height: 54,
                    subImage: { x: 39, y: 0, width: 30, height: 54 },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    backgroundColor: { red: 255, green: 0, blue: 0},
                    alpha: 1
                });

var swatch2 = Overlays.addOverlay({
                    x: 130,
                    y: 200,
                    width: 31,
                    height: 54,
                    subImage: { x: 39, y: 55, width: 30, height: 54 },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    backgroundColor: { red: 0, green: 255, blue: 0},
                    alpha: 1.0
                });

var swatch3 = Overlays.addOverlay({
                    x: 160,
                    y: 200,
                    width: 31,
                    height: 54,
                    subImage: { x: 39, y: 0, width: 30, height: 54 },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    backgroundColor: { red: 0, green: 0, blue: 255},
                    alpha: 1.0
                });

var swatch4 = Overlays.addOverlay({
                    x: 190,
                    y: 200,
                    width: 31,
                    height: 54,
                    subImage: { x: 39, y: 0, width: 30, height: 54 },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    backgroundColor: { red: 0, green: 255, blue: 255},
                    alpha: 1.0
                });

var swatch5 = Overlays.addOverlay({
                    x: 220,
                    y: 200,
                    width: 31,
                    height: 54,
                    subImage: { x: 39, y: 0, width: 30, height: 54 },
                    imageURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/testing-swatches.svg",
                    backgroundColor: { red: 255, green: 255, blue: 0},
                    alpha: 1.0
                });

var toolA = Overlays.addOverlay({
                    x: 100,
                    y: 100,
                    width: 62,
                    height: 40,
                    subImage: { x: 0, y: 0, width: 62, height: 40 },
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/hifi-interface-tools.svg",
                    backgroundColor: { red: 255, green: 255, blue: 255},
                    alpha: 0.7,
                    visible: false
                });

var slider = Overlays.addOverlay({
                    // alternate form of expressing bounds
                    bounds: { x: 100, y: 300, width: 158, height: 35},
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/slider.png",
                    backgroundColor: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

var thumb = Overlays.addOverlay({
                    x: 130,
                    y: 309,
                    width: 18,
                    height: 17,
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/thumb.png",
                    backgroundColor: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

      
// 270x 109
// 109... 109/2 = 54,1,54
// 270... 39 to 66 = 28 x 9 swatches with 
// unselected:
//    38,0,28,54
// selected:
//    38,55,28,54
//http://highfidelity-public.s3-us-west-1.amazonaws.com/images/swatches.svg

// 123456789*123456789*123456789*
// 0123456789*123456789*123456789*

function scriptEnding() {
    Overlays.deleteOverlay(toolA);
    //Overlays.deleteOverlay(toolB);
    //Overlays.deleteOverlay(toolC);
    Overlays.deleteOverlay(swatch1);
    Overlays.deleteOverlay(swatch2);
    Overlays.deleteOverlay(swatch3);
    Overlays.deleteOverlay(swatch4);
    Overlays.deleteOverlay(swatch5);
    Overlays.deleteOverlay(thumb);
    Overlays.deleteOverlay(slider);
}
Script.scriptEnding.connect(scriptEnding);


var toolAVisible = false;
var minX = 130;
var maxX = minX + 65;
var moveX = (minX + maxX)/2;
var dX = 1;
function update() {
    moveX = moveX + dX;
    if (moveX > maxX) {
        dX = -1;
    }
    if (moveX < minX) {
        dX = 1;
        if (toolAVisible) {
            toolAVisible = false;
        } else {
            toolAVisible = true;
        }
        Overlays.editOverlay(toolA, { visible: toolAVisible } );
    }
    Overlays.editOverlay(thumb, { x: moveX } );

}
Script.willSendVisualDataCallback.connect(update);
