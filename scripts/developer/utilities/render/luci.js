"use strict";

//
//  Luci.js
//  tablet-engine app
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var AppUi = Script.require('appUi');

    var moveDebugCursor = false;
    var onMousePressEvent = function (e) {
        if (e.isMiddleButton) {
            moveDebugCursor = true;
            setDebugCursor(e.x, e.y);
        }
    };
    Controller.mousePressEvent.connect(onMousePressEvent);

    var onMouseReleaseEvent = function () {
        moveDebugCursor = false;
    };
    Controller.mouseReleaseEvent.connect(onMouseReleaseEvent);

    var onMouseMoveEvent = function (e) {
        if (moveDebugCursor) {
            setDebugCursor(e.x, e.y);
        }
    };
    Controller.mouseMoveEvent.connect(onMouseMoveEvent);

    function setDebugCursor(x, y) {
        var nx = 2.0 * (x / Window.innerWidth) - 1.0;
        var ny = 1.0 - 2.0 * ((y) / (Window.innerHeight));

        Render.getConfig("RenderMainView").getConfig("DebugDeferredBuffer").size = { x: nx, y: ny, z: 1.0, w: 1.0 };
    }

    function Page(title, qmlurl, width, height) {
        this.title = title;
        this.qml = qmlurl;
        this.width = width;
        this.height = height;

        this.window;

        print("Page: New Page:" + JSON.stringify(this));
    }

    Page.prototype.killView = function () {
        print("Page: Kill window for page:" + JSON.stringify(this));
        if (this.window) { 
            print("Page: Kill window for page:" + this.title);
            //this.window.closed.disconnect(function () {
            //    this.killView();
            //});
            this.window.close();
            this.window = false;
        }
    };

    Page.prototype.createView = function () {
        var that = this;
        if (!this.window) {
            print("Page: New window for page:" + this.title);
            this.window = Desktop.createWindow(Script.resolvePath(this.qml), {
                title: this.title,
                presentationMode: Desktop.PresentationMode.NATIVE,
                size: {x: this.width, y: this.height}
            });
            this.window.closed.connect(function () {
                that.killView();
            });
        }  
    };


    var Pages = function () {
        this._pages = {};
    };

    Pages.prototype.addPage = function (command, title, qmlurl, width, height) {
        this._pages[command] = new Page(title, qmlurl, width, height);
    };

    Pages.prototype.open = function (command) {
        print("Pages: command = " + command);
        if (!this._pages[command]) {
            print("Pages: unknown command = " + command);
            return;
        }
        this._pages[command].createView();
    };

    Pages.prototype.clear = function () {
        for (var p in this._pages) {
            print("Pages: kill page: " + p);
            this._pages[p].killView();
            delete this._pages[p];
        }
        this._pages = {};
    };
    var pages = new Pages();

    pages.addPage('openEngineView', 'Render Engine', 'engineInspector.qml', 300, 400);
    pages.addPage('openEngineLODView', 'Render LOD', 'lod.qml', 300, 400);
    pages.addPage('openCullInspectorView', 'Cull Inspector', 'culling.qml', 300, 400);

    function fromQml(message) {
        if (pages.open(message.method)) {
            return;
        }    
    }

    var ui;
    function startup() {
        ui = new AppUi({
            buttonName: "LUCI",
            home: Script.resolvePath("deferredLighting.qml"),
            additionalAppScreens: Script.resolvePath("engineInspector.qml"),
            onMessage: fromQml,
            normalButton: Script.resolvePath("../../../system/assets/images/luci-i.svg"),
            activeButton: Script.resolvePath("../../../system/assets/images/luci-a.svg")
        });
    }
    startup();
    Script.scriptEnding.connect(function () {
        Controller.mousePressEvent.disconnect(onMousePressEvent);
        Controller.mouseReleaseEvent.disconnect(onMouseReleaseEvent);
        Controller.mouseMoveEvent.disconnect(onMouseMoveEvent);
        pages.clear();
        // killEngineInspectorView();
        // killCullInspectorView();
        // killEngineLODWindow();
    });
}()); 
