//
//  stats.js
//  scripts/developer/utilities/audio
//
//  Zach Pomerantz, created on 9/22/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

if (HMD.active && !Settings.getValue("HUDUIEnabled")) {
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var qml = Script.resolvePath("TabletStats.qml");
    tablet.loadQMLSource(qml);
    Script.stop();

} else {
    var INITIAL_WIDTH = 400;
    var INITIAL_OFFSET = 50;

    var qml = Script.resolvePath("Stats.qml");
    var window = new OverlayWindow({
        title: "Audio Interface Statistics",
        source: qml,
        width: 500, height: 520 // stats.qml may be too large for some screens
    });
    window.setPosition(INITIAL_OFFSET, INITIAL_OFFSET);

    window.closed.connect(function () { Script.stop(); });
}
