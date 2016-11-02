import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1;
import Qt.labs.settings 1.0
import HFWebEngineProfile 1.0

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

    // The tool window, one instance
    property alias toolWindow: toolWindow
    ToolWindow { id: toolWindow }

    property var browserProfile: HFWebEngineProfile {
        id: webviewProfile
        storageName: "qmlWebEngine"
    }

    Action {
        text: "Open Browser"
        shortcut: "Ctrl+B"
        onTriggered: {
            console.log("Open browser");
            browserBuilder.createObject(desktop);
        }
        property var browserBuilder: Component {
            Browser{}
        }
    }

    Component { id: toolbarBuilder; Toolbar { } }
    // This used to create sysToolbar dynamically with a call to getToolbar() within onCompleted.
    // Beginning with QT 5.6, this stopped working, as anything added to toolbars too early got
    // wiped during startup.
    Toolbar {
        id: sysToolbar;
        objectName: "com.highfidelity.interface.toolbar.system";
        // Literal 50 is overwritten by settings from previous session, and sysToolbar.x comes from settings.
        x: settings.firstRun ? (desktop.width - sysToolbar.width) / 2 : sysToolbar.x;
        y: 50
    }
    property var toolbars: (function (map) { // answer dictionary preloaded with sysToolbar
        map[sysToolbar.objectName] = sysToolbar;
        return map; })({});

    Settings {
        id: settings
        property bool firstRun: true
    }
    Component.onCompleted: {
        WebEngine.settings.javascriptCanOpenWindows = true;
        WebEngine.settings.javascriptCanAccessClipboard = false;
        WebEngine.settings.spatialNavigationEnabled = false;
        WebEngine.settings.localContentCanAccessRemoteUrls = true;

        [ // Allocate the standard buttons in the correct order. They will get images, etc., via scripts.
            "hmdToggle", "mute", "mod", "help",
            "hudToggle",
            "com.highfidelity.interface.system.editButton", "marketplace", "snapshot", "goto"
        ].forEach(function (name) {
            sysToolbar.addButton({objectName: name});
        });
        var toggleHudButton = sysToolbar.findButton("hudToggle");
        toggleHudButton.imageURL = "../../../icons/hud.svg";
        toggleHudButton.pinned = true;
        sysToolbar.updatePinned(); // automatic when adding buttons only IFF button is pinned at creation.

        toggleHudButton.buttonState = Qt.binding(function(){
            return desktop.pinned ? 1 : 0
        });
        toggleHudButton.clicked.connect(function(){
            console.log("Clicked on hud button")
            var overlayMenuItem = "Overlays"
            MenuInterface.setIsOptionChecked(overlayMenuItem, !MenuInterface.isOptionChecked(overlayMenuItem));
        });
    }

    // Accept a download through the webview
    property bool webViewProfileSetup: false
    property string currentUrl: ""
    property string adaptedPath: ""
    property string tempDir: ""

    function initWebviewProfileHandlers(profile) {
        console.log("The webview url in desktop is: " + currentUrl);
        if (webViewProfileSetup) return;
        webViewProfileSetup = true;

        profile.downloadRequested.connect(function(download){
            console.log("Download start: " + download.state);
            adaptedPath = File.convertUrlToPath(currentUrl);
            tempDir = File.getTempDir();
            console.log("Temp dir created: " + tempDir);
            download.path = tempDir + "/" + adaptedPath;
            console.log("Path where object should download: " + download.path);
            download.accept();
            if (download.state === WebEngineDownloadItem.DownloadInterrupted) {
                console.log("download failed to complete");
            }
        })

        profile.downloadFinished.connect(function(download){
            if (download.state === WebEngineDownloadItem.DownloadCompleted) {
                File.runUnzip(download.path, currentUrl);
            } else {
                console.log("The download was corrupted, state: " + download.state);
            }
        })
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
