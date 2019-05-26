// Test key commands
PlatformInfo.getComputer()
// {"OS":"WINDOWS","keys":null,"model":"","profileTier":"HIGH","vendor":""}
PlatformInfo.getNumCPUs()
// 1
PlatformInfo.getCPU(0)
//{"clockSpeed":" 4.00GHz","model":") i7-6700K CPU @ 4.00GHz","numCores":8,"vendor":"Intel(R) Core(TM) i7-6700K CPU @ 4.00GHz"}
PlatformInfo.getNumGPUs()
// 1
PlatformInfo.getGPU(0)
// {"driver":"25.21.14.1967","model":"NVIDIA GeForce GTX 1080","vendor":"NVIDIA GeForce GTX 1080","videoMemory":8079}

var Page = Script.require('./luci/Page.js');


function openView() {
    var pages = new Pages();
    function fromQml(message) {
        if (pages.open(message.method)) {
            return;
        }    
    }

    var platformWindow  

    function closeLuciWindow() {
        if (luciWindow !== undefined) {
            activeWindow.fromQml.disconnect(fromQml);
        }
        luciWindow = {};

        Controller.mousePressEvent.disconnect(onMousePressEvent);
        Controller.mouseReleaseEvent.disconnect(onMouseReleaseEvent);
        Controller.mouseMoveEvent.disconnect(onMouseMoveEvent);
        pages.clear();
    }

    pages.addPage('Platform', 'Platform', '../platform.qml', 350, 700);
    pages.open('Platform');

    
    return pages;
}


openView();
