//
//  hotPlaces.js 
//
//  Press the number keys to jump to different places in the metaverse!
//  
//  Created by Philip Rosedale on November 20, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


Controller.keyPressEvent.connect(function (event) {
    if (event.text == "1") {
        Window.location = "hifi://starchamber";
    } else if (event.text == "2") {
        Window.location = "hifi://apartment";
    } else if (event.text == "3") {
        Window.location = "hifi://rivenglen";
    } else if (event.text == "4") {
        Window.location = "hifi://sanfrancisco";
    } else if (event.text == "5") {
        Window.location = "hifi://porto";
    } else if (event.text == "6") {
        Window.location = "hifi://hoo";
    } 
});
