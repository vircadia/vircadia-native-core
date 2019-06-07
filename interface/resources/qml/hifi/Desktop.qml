import QtQuick 2.7
import Qt.labs.settings 1.0 as QtSettings

import QtQuick.Controls 2.3

import "../desktop" as OriginalDesktop
import ".."
import "."
import "./toolbars"
import controlsUit 1.0

OriginalDesktop.Desktop {
    id: desktop

    MouseArea {
        id: hoverWatch
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        scrollGestureEnabled: false // we don't need/want these
        onEntered: if (typeof ApplicationCompositor !== "undefined") ApplicationCompositor.reticleOverDesktop = true
        onExited: if (typeof ApplicationCompositor !== "undefined") ApplicationCompositor.reticleOverDesktop = false
        acceptedButtons: Qt.NoButton
    }

    Action {
        text: "Open Browser"
        onTriggered: {
            console.log("Open browser");
            browserBuilder.createObject(desktop);
        }
        property var browserBuilder: Component {
            Browser{}
        }
    }

    onHeightChanged: {
        if (height > 100) {
            adjustToolbarPosition();
        }
    }

    function adjustToolbarPosition() {
        var toolbar = getToolbar("com.highfidelity.interface.toolbar.system")
        // check if Y position was changed, if toolbar height is assigned etc
        // default position is adjusted to border width
        var toolbarY = toolbar.settings.y > 6 ?
                    toolbar.settings.y :
                    desktop.height - (toolbar.height > 0 ? toolbar.height : 50)  - 56

        if (toolbar.settings.desktopHeight > 100 && desktop.height !== toolbar.settings.desktopHeight) {
            toolbarY += desktop.height - toolbar.settings.desktopHeight
        }

        toolbar.y = toolbarY
        toolbar.settings.y = toolbarY
        toolbar.settings.desktopHeight = desktop.height
    }

    Component { id: toolbarBuilder; Toolbar { } }
    // This used to create sysToolbar dynamically with a call to getToolbar() within onCompleted.
    // Beginning with QT 5.6, this stopped working, as anything added to toolbars too early got
    // wiped during startup.
    Toolbar {
        id: sysToolbar;
        objectName: "com.highfidelity.interface.toolbar.system";
        property var tablet: Tablet.getTablet("com.highfidelity.interface.tablet.system");
        anchors.horizontalCenter: settings.constrainToolbarToCenterX ? desktop.horizontalCenter : undefined;
        // Literal 50 is overwritten by settings from previous session, and sysToolbar.x comes from settings when not constrained.
        x: sysToolbar.x
        buttonModel: tablet ? tablet.buttons : null;
        shown: tablet ? tablet.toolbarMode : false;
    }

    QtSettings.Settings {
        id: settings;
        category: "toolbar";
        property bool constrainToolbarToCenterX: true;
    }
    function setConstrainToolbarToCenterX(constrain) { // Learn about c++ preference change.
        settings.constrainToolbarToCenterX = constrain;
    }
    property var toolbars: (function (map) { // answer dictionary preloaded with sysToolbar
        map[sysToolbar.objectName] = sysToolbar;
        return map;
    })({});

    Component.onCompleted: {
        webEngineConfig.setupWebEngineSettings();
    }

    // Accept a download through the webview
    property alias webViewProfileSetup: webEngineConfig.webViewProfileSetup
    property alias currentUrl: webEngineConfig.currentUrl
    property alias downloadUrl: webEngineConfig.downloadUrl
    property alias adaptedPath: webEngineConfig.adaptedPath
    property alias tempDir: webEngineConfig.tempDir
    property var initWebviewProfileHandlers: webEngineConfig.initWebviewProfileHandlers
    property bool autoAdd: false

    DesktopWebEngine {
        id: webEngineConfig
    }

    function setAutoAdd(auto) {
        autoAdd = auto;
    }

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
