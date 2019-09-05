"use strict";

//  Created by Bradley Austin Davis on 2019/08/29
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var QML_URL = "qrc:/qml/hifi/tablet/DynamicWebview.qml"
    var LOCAL_FILE_URL = "file:///C:/test-file.html"

    var _this = this;
    _this.preload = function (pEntityID) {
        var window = Desktop.createWindow(QML_URL, {
            title: "Local file access test",
            additionalFlags: Desktop.ALWAYS_ON_TOP,
            presentationMode: Desktop.PresentationMode.NATIVE,
            size: { x: 640, y: 480 },
            visible: true,
            position: { x: 100, y: 100 },
        });
        window.sendToQml({ url: LOCAL_FILE_URL });
    };

    _this.unload = function () {
        console.log("QQQ on preload");
    };
});
