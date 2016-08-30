import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1;
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

    // The tool window, one instance
    property alias toolWindow: toolWindow
    ToolWindow { id: toolWindow }

    property var browserProfile: WebEngineProfile {
        id: webviewProfile
        httpUserAgent: "Chrome/48.0 (HighFidelityInterface)"
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

    property var toolbars: ({})
    Component { id: toolbarBuilder; Toolbar { } }

    Component.onCompleted: {
        WebEngine.settings.javascriptCanOpenWindows = true;
        WebEngine.settings.javascriptCanAccessClipboard = false;
        WebEngine.settings.spatialNavigationEnabled = false;
        WebEngine.settings.localContentCanAccessRemoteUrls = true;

        var sysToolbar = desktop.getToolbar("com.highfidelity.interface.toolbar.system");
        var toggleHudButton = sysToolbar.addButton({
            objectName: "hudToggle",
            imageURL: "../../../icons/hud.svg",
            visible: true,
            pinned: true,
        });

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


