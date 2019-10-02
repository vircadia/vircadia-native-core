"use strict";
var Page = Script.require('../lib/skit/Page.js');

function openView() {
    //window.closed.connect(function() { Script.stop(); });


    var pages = new Pages(Script.resolvePath("."));  
    function fromQml(message) {
        console.log(JSON.stringify(message))
        if (message.method == "inspectResource") {
            pages.open("openResourceInspector")
            pages.sendTo("openResourceInspector", message)
            return;
        } 
        if (pages.open(message.method)) {
            return;
        }    
    }

    function openCashWindow(window) {
        var onMousePressEvent = function (e) {
        };
        Controller.mousePressEvent.connect(onMousePressEvent);
    
        var onMouseReleaseEvent = function () {
        };
        Controller.mouseReleaseEvent.connect(onMouseReleaseEvent);
    
        var onMouseMoveEvent = function (e) {
        };
        Controller.mouseMoveEvent.connect(onMouseMoveEvent);
    }

    function closeCashWindow() {
        Controller.mousePressEvent.disconnect(onMousePressEvent);
        Controller.mouseReleaseEvent.disconnect(onMouseReleaseEvent);
        Controller.mouseMoveEvent.disconnect(onMouseMoveEvent);
        pages.clear();
    }
 

    pages.addPage('Cash', 'Cash', "cash.qml", 300, 500, fromQml, openCashWindow, closeCashWindow);
    pages.addPage('openModelCacheInspector', 'Model Cache Inspector', "cash/ModelCacheInspector.qml", 300, 500, fromQml);
    pages.addPage('openMaterialCacheInspector', 'Material Cache Inspector', "cash/MaterialCacheInspector.qml", 300, 500, fromQml);
    pages.addPage('openTextureCacheInspector', 'Texture Cache Inspector', "cash/TextureCacheInspector.qml", 300, 500, fromQml);
    pages.addPage('openAnimationCacheInspector', 'Animation Cache Inspector', "cash/AnimationCacheInspector.qml", 300, 500);
    pages.addPage('openSoundCacheInspector', 'Sound Cache Inspector', "cash/SoundCacheInspector.qml", 300, 500);
    pages.addPage('openResourceInspector', 'Resource Inspector', "cash/ResourceInspector.qml", 300, 500);


    pages.open('Cash');

    
    return pages;
}


openView();
