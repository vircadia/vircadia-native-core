import QtQuick 2.5
import QtQuick.Controls 1.4

import Qt.labs.settings 1.0

import "../desktop" as OriginalDesktop
import ".."
import "."
import "./toolbars"

OriginalDesktop.Desktop {
    id: desktop

    MouseArea {
        id: hoverWatch
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        scrollGestureEnabled: false // we don't need/want these
        onEntered: ApplicationCompositor.reticleOverDesktop = true
        onExited: ApplicationCompositor.reticleOverDesktop = false
        acceptedButtons: Qt.NoButton
		

    }


    Component { id: toolbarBuilder; Toolbar { } }
    // This used to create sysToolbar dynamically with a call to getToolbar() within onCompleted.
    // Beginning with QT 5.6, this stopped working, as anything added to toolbars too early got
    // wiped during startup.
    Toolbar {
        id: sysToolbar;
        objectName: "com.highfidelity.interface.toolbar.system";
        // Magic: sysToolbar.x and y come from settings, and are bound before the properties specified here are applied.
        x: sysToolbar.x;
        y: sysToolbar.y;
    }
    property var toolbars: (function (map) { // answer dictionary preloaded with sysToolbar
        map[sysToolbar.objectName] = sysToolbar;
        return map; })({});

    Component.onCompleted: {
    }

    // Accept a download through the webview
    property bool webViewProfileSetup: false
    property string currentUrl: ""
    property string adaptedPath: ""
    property string tempDir: ""

    // Create or fetch a toolbar with the given name
    function getToolbar(name) {
        var result = toolbars[name];
        if (!result) {
            result = toolbars[name] = toolbarBuilder.createObject(desktop, {});
            result.objectName = name;
        }
        return result;
    }
}


