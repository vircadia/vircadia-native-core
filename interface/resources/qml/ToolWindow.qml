import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1

import Qt.labs.settings 1.0

import "windows" as Windows
import "controls" as Controls

Windows.Window {
    id: toolWindow
    resizable: true
    objectName: "ToolWindow"
    destroyOnCloseButton: false
    destroyOnInvisible: false
    closable: true
    visible: false
    width: 384; height: 640;
    title: "Tools"
    property alias tabView: tabView
    onParentChanged: {
        if (parent) {
            x = 120;
            y = 120;
        }
    }

    Settings {
        category: "ToolWindow.Position"
        property alias x: toolWindow.x
        property alias y: toolWindow.y
    }

    TabView {
        anchors.fill: parent
        id: tabView;
        Repeater {
            model: 4
            Tab {
                // Force loading of the content even if the tab is not visible
                // (required for letting the C++ code access the webview)
                active: true
                enabled: false
                property string originalUrl: "";

                Controls.WebView {
                    id: webView;
                    // we need to store the original url here for future identification
                    // A unique identifier to let the HTML JS find the event bridge 
                    // object (our C++ wrapper)
                    property string uid;
                    anchors.fill: parent
                    enabled: false

                    // This is for JS/QML communication, which is unused in a WebWindow,
                    // but not having this here results in spurious warnings about a 
                    // missing signal
                    signal sendToScript(var message);
                    
                    onUrlChanged: webView.runJavaScript("EventBridgeUid = \"" + uid + "\";");
                    onEnabledChanged: toolWindow.updateVisiblity();
                    onLoadingChanged: {
                        if (loadRequest.status == WebEngineView.LoadSucceededStatus) {
                            webView.runJavaScript("EventBridgeUid = \"" + uid + "\";");
                        }
                    }
                }
            }
        }
    }

    function updateVisiblity() {
        for (var i = 0; i < tabView.count; ++i) {
            if (tabView.getTab(i).enabled) {
                return;
            }
        }
        visible = false;
    }

    function findIndexForUrl(source) {
        for (var i = 0; i < tabView.count; ++i) {
            var tab = tabView.getTab(i);
            if (tab.originalUrl === source) {
                return i;
            }
        }
        console.warn("Could not find tab for " + source);
        return -1;
    }

    function findTabForUrl(source) {
        var index = findIndexForUrl(source);
        if (index < 0) {
            return;
        }
        return tabView.getTab(index);
    }

    function showTabForUrl(source, newVisible) {
        var index = findIndexForUrl(source);
        if (index < 0) {
            return;
        }

        var tab = tabView.getTab(index);
        if (newVisible) {
            toolWindow.visible = true
            tab.enabled = true
        } else {
            tab.enabled = false;
            updateVisiblity();
        }
    }

    function findFreeTab() {
        for (var i = 0; i < tabView.count; ++i) {
            var tab = tabView.getTab(i);
            if (tab && (!tab.originalUrl || tab.originalUrl === "")) {
                return i;
            }
        }
        console.warn("Could not find free tab");
        return -1;
    }

    function removeTabForUrl(source) {
        var index = findIndexForUrl(source);
        if (index < 0) {
            return;
        }

        var tab = tabView.getTab(index);
        tab.title = "";
        tab.enabled = false;
        tab.originalUrl = "";
        tab.item.url = "about:blank";
        tab.item.enabled = false;
    }

    function addWebTab(properties) {
        if (!properties.source) {
            console.warn("Attempted to open Web Tool Pane without URL");
            return;
        }

        var existingTabIndex = findIndexForUrl(properties.source);
        if (existingTabIndex >= 0) {
            console.log("Existing tab " + existingTabIndex + " found with URL " + properties.source);
            var tab = tabView.getTab(existingTabIndex);
            return tab.item;
        }

        var freeTabIndex = findFreeTab();
        if (freeTabIndex === -1) {
            console.warn("Unable to add new tab");
            return;
        }

        if (properties.width) {
            tabView.width = Math.min(Math.max(tabView.width, properties.width), toolWindow.maxSize.x);
        }

        if (properties.height) {
            tabView.height = Math.min(Math.max(tabView.height, properties.height), toolWindow.maxSize.y);
        }

        var tab = tabView.getTab(freeTabIndex);
        tab.title = properties.title || "Unknown";
        tab.enabled = true;
        tab.originalUrl = properties.source;

        var result = tab.item;
        result.enabled = true;
        result.url = properties.source;
        return result;
    }
}
