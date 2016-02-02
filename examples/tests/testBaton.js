"use strict";
/*jslint nomen: true, plusplus: true, vars: true*/
var Vec3, Quat, MyAvatar, Entities, Camera, Script, print;

Script.include("../libraries/virtualBaton.18.js");
var TICKER_INTERVAL = 1000; // ms
var baton = virtualBaton({key: 'io.highfidelity.testBaton'});
var ticker, countDown;

// Tick every TICKER_INTERVAL.
function gotBaton(key) {
    print("gotBaton", key);
    countDown = 20;
    ticker = Script.setInterval(function () {
        print("tick");
    }, 1000);
}
// If we've lost the baton (e.g., to network problems), stop ticking
// but ask for the baton back (waiting indefinitely to get it).
function lostBaton(key) {
    print("lostBaton", key);
    Script.clearInterval(ticker);
    baton.claim(gotBaton, lostBaton);
}
baton.claim(gotBaton, lostBaton);

    
