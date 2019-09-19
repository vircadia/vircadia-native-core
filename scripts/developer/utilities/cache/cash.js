"use strict";
var Page = Script.require('./cash/Page.js');

function openView() {
    //window.closed.connect(function() { Script.stop(); });


    var pages = new Pages();
    function fromQml(message) {
        console.log(JSON.stringify(message))
        if (pages.open(message.method)) {
            return;
        }    
    }

    var cashWindow  
    function openCashWindow(window) {
        if (cashWindow !== undefined) {
            activeWindow.fromQml.disconnect(fromQml);
        }
        if (window !== undefined) {
            window.fromQml.connect(fromQml);
        }
        cashWindow = window;


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
        if (cashWindow !== undefined) {
            activeWindow.fromQml.disconnect(fromQml);
        }
        cashWindow = {};

        Controller.mousePressEvent.disconnect(onMousePressEvent);
        Controller.mouseReleaseEvent.disconnect(onMouseReleaseEvent);
        Controller.mouseMoveEvent.disconnect(onMouseMoveEvent);
        pages.clear();
    }

    pages.addPage('Cash', 'Cash', "../cash.qml", 300, 500, openCashWindow, closeCashWindow);
    pages.addPage('openModelCacheInspector', 'Model Cache Inspector', "./ModelCacheInspector.qml", 300, 500);
    pages.addPage('openMaterialCacheInspector', 'Material Cache Inspector', "./MaterialCacheInspector.qml", 300, 500);
    pages.addPage('openTextureCacheInspector', 'Texture Cache Inspector', "./TextureCacheInspector.qml", 300, 500);
    pages.addPage('openAnimationCacheInspector', 'Animation Cache Inspector', "./AnimationCacheInspector.qml", 300, 500);
    pages.addPage('openSoundCacheInspector', 'Sound Cache Inspector', "./SoundCacheInspector.qml", 300, 500);

    pages.open('Cash');

    
    return pages;
}


openView();
