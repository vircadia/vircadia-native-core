
import QtQuick 2.3
import QtQuick.Controls 1.4
import QtWebChannel 1.0
import QtWebEngine 1.2
import QtWebSockets 1.0

import "windows" as Windows
import "controls"
import "controls-uit" as Controls
import "styles"
import "styles-uit"

Windows.Window {
    id: root
    HifiConstants { id: hifi }
    title: "QmlWindow"
    resizable: true
    shown: false
    focus: true
    property var channel;
    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false
    property var source;
    property var dynamicContent;

    // Keyboard control properties in case needed by QML content.
    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onSourceChanged: {
        if (dynamicContent) {
            dynamicContent.destroy();
            dynamicContent = null; 
        }
        QmlSurface.load(source, contentHolder, function(newObject) {
            dynamicContent = newObject;
        });
    }

    // Handle message traffic from the script that launched us to the loaded QML
    function fromScript(message) {
        if (root.dynamicContent && root.dynamicContent.fromScript) {
            root.dynamicContent.fromScript(message);
        }
    }

    function clearDebugWindow() {
        if (root.dynamicContent && root.dynamicContent.clearWindow) {
            root.dynamicContent.clearWindow();
        }
    }
    
    // Handle message traffic from our loaded QML to the script that launched us
    signal sendToScript(var message);
    onDynamicContentChanged: {
        if (dynamicContent && dynamicContent.sendToScript) {
            dynamicContent.sendToScript.connect(sendToScript);
        }
    }

    Item {
        id: contentHolder
        anchors.fill: parent
    }
}
