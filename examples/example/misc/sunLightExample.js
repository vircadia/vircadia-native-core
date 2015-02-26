//
//  SunLightExample.js
//  examples
//  Sam Gateau
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var intensity = 1.0;
var day = 0.0;
var hour = 12.0;
var longitude = -115.0;
var latitude = -31.0;

Scene.setDayTime(hour);
//Scene.setOriginLocation(longitude, latitude, 0.0);

function ticktack() {
	hour += 0.1;
    //Scene.setSunIntensity(Math.cos(time));
    if (hour > 24.0) {
        hour = 0.0;
        day++;
        Scene.setYearTime(day);
    }
    Scene.setDayTime(hour);
}

Script.setInterval(ticktack, 41);
