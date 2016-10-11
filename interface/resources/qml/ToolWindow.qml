//
//  ToolWindow.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtWebEngine 1.1
import QtWebChannel 1.0
import Qt.labs.settings 1.0

import "windows"
import "controls-uit"
import "styles-uit"


ScrollingWindow {
    id: toolWindow
    resizable: true
    objectName: "ToolWindow"
    destroyOnCloseButton: false
    destroyOnHidden: false
    closable: true
    shown: false
    title: "Edit"
    property alias tabView: tabView
    implicitWidth: 520; implicitHeight: 695
    minSize: Qt.vector2d(456, 500)

    HifiConstants { id: hifi }

    onParentChanged: {
        if (parent) {
            x = 120;
            y = 120;
        }
    }

    onShownChanged: {
        keyboardEnabled = HMD.active;
    }

    Settings {
        category: "ToolWindow.Position"
        property alias x: toolWindow.x
        property alias y: toolWindow.y
    }

    Item {
        id: toolWindowTabViewItem
        height: pane.scrollHeight
        width: pane.contentWidth
        anchors.left: parent.left
        anchors.top: parent.top

        TabView {
            id: tabView
            width: pane.contentWidth
            // Pane height so that don't use Window's scrollbars otherwise tabs may be scrolled out of view.
            height: pane.scrollHeight
            property int tabCount: 0

            Repeater {
                model: 4
                Tab {
                    // Force loading of the content even if the tab is not visible
                    // (required for letting the C++ code access the webview)
                    active: true
                    enabled: false
                    property string originalUrl: ""

                    WebView {
                        id: webView
                        anchors.fill: parent
                        enabled: false
                        property alias eventBridgeWrapper: eventBridgeWrapper

                        QtObject {
                            id: eventBridgeWrapper
                            WebChannel.id: "eventBridgeWrapper"
                            property var eventBridge
                        }

                        webChannel.registeredObjects: [eventBridgeWrapper]
                        onEnabledChanged: toolWindow.updateVisiblity()
                    }
                }
            }

            style: TabViewStyle {

                frame: Rectangle {  // Background shown before content loads.
                    anchors.fill: parent
                    color: hifi.colors.baseGray
                }

                frameOverlap: 0

                tab: Rectangle {
                    implicitWidth: text.width
                    implicitHeight: 3 * text.height
                    color: styleData.selected ? hifi.colors.black : hifi.colors.tabBackgroundDark

                    RalewayRegular {
                        id: text
                        text: styleData.title
                        font.capitalization: Font.AllUppercase
                        size: hifi.fontSizes.tabName
                        width: tabView.tabCount > 1 ? styleData.availableWidth / tabView.tabCount : implicitWidth + 2 * hifi.dimensions.contentSpacing.x
                        elide: Text.ElideRight
                        color: styleData.selected ? hifi.colors.primaryHighlight : hifi.colors.lightGrayText
                        horizontalAlignment: Text.AlignHCenter
                        anchors.centerIn: parent
                    }

                    Rectangle {  // Separator.
                        width: 1
                        height: parent.height
                        color: hifi.colors.black
                        anchors.left: parent.left
                        anchors.top: parent.top
                        visible: styleData.index > 0

                        Rectangle {
                            width: 1
                            height: 1
                            color: hifi.colors.baseGray
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                        }
                    }

                    Rectangle {  // Active underline.
                        width: parent.width - (styleData.index > 0 ? 1 : 0)
                        height: 1
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        color: styleData.selected ? hifi.colors.primaryHighlight : hifi.colors.baseGray
                    }
                }

                tabOverlap: 0
            }
        }
    }

    function updateVisiblity() {
        if (visible) {
            for (var i = 0; i < tabView.count; ++i) {
                if (tabView.getTab(i).enabled) {
                    return;
                }
            }
            shown = false;
        }
    }

    function findIndexForUrl(source) {
        for (var i = 0; i < tabView.count; ++i) {
            var tab = tabView.getTab(i);
            if (tab.originalUrl === source) {
                return i;
            }
        }
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
            toolWindow.shown = true
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
        tabView.tabCount--;
    }

    function addWebTab(properties) {
        if (!properties.source) {
            console.warn("Attempted to open Web Tool Pane without URL");
            return;
        }

        var existingTabIndex = findIndexForUrl(properties.source);
        if (existingTabIndex >= 0) {
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

        var eventBridge = properties.eventBridge;

        var result = tab.item;
        result.enabled = true;
        tabView.tabCount++;
        result.eventBridgeWrapper.eventBridge = eventBridge;
        result.url = properties.source;
        return result;
    }
}
