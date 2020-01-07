//
//  WindowRoot.qml
//
//  Created by Anthony Thibault on 14 Feb 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This qml is used when tablet content is shown on the 2d overlay ui
//  TODO: FIXME: this is practically identical to TabletRoot.qml

import "../../windows" as Windows
import QtQuick 2.0
import Hifi 1.0

import Qt.labs.settings 1.0

Windows.ScrollingWindow {
    id: tabletRoot
    objectName: "tabletRoot"
    property string username: "Unknown user"
    signal screenChanged(var type, var url);

    property var rootMenu;
    property string subMenu: ""

    shown: false
    resizable: false

    Settings {
        id: settings
        category: "WindowRoot.Windows"
        property real width: 480
        property real height: 706
    }

    onResizableChanged: {
        if (!resizable) {
            // restore default size
            settings.width = tabletRoot.width
            settings.height = tabletRoot.height
            tabletRoot.width = 480
            tabletRoot.height = 706
        } else {
            tabletRoot.width = settings.width
            tabletRoot.height = settings.height
        }
    }

    signal showDesktop();

    function setResizable(value) {
        tabletRoot.resizable = value;
    }

    function setMenuProperties(rootMenu, subMenu) {
        tabletRoot.rootMenu = rootMenu;
        tabletRoot.subMenu = subMenu;
    }

    function loadSource(url) {
        loader.load(url) 
    }

    function loadWebContent(source, url, injectJavaScriptUrl) {
        loader.load(source, function() {
            loader.item.scriptURL = injectJavaScriptUrl;
            loader.item.url = url;
            if (loader.item.hasOwnProperty("closeButtonVisible")) {
                loader.item.closeButtonVisible = false;
            }
            
            screenChanged("Web", url);
        });
    }

    function loadWebBase(url, injectJavaScriptUrl) {
        loadWebContent("hifi/tablet/TabletWebView.qml", url, injectJavaScriptUrl);
    }

    function loadTabletWebBase(url, injectJavaScriptUrl) {
        loadWebContent("hifi/tablet/BlocksWebView.qml", url, injectJavaScriptUrl);
    }

    // used to send a message from qml to interface script.
    signal sendToScript(var message);

    // used to receive messages from interface script
    function fromScript(message) {
        if (loader.item !== null) {
            if (loader.item.hasOwnProperty("fromScript")) {
                loader.item.fromScript(message);
            }
        }
    }

    SoundEffect {
        id: buttonClickSound
        volume: 0.1
        source: "../../../sounds/Gamemaster-Audio-button-click.wav"
    }

    function playButtonClickSound() {
        // Because of the asynchronous nature of initalization, it is possible for this function to be
        // called before the C++ has set the globalPosition context variable.
        if (typeof globalPosition !== 'undefined') {
            buttonClickSound.play(globalPosition);
        }
    }

    function setUsername(newUsername) {
        username = newUsername;
    }

    // Hook up callback for clara.io download from the marketplace.
    Connections {
        id: eventBridgeConnection
        target: eventBridge
        onWebEventReceived: {
            if (typeof message === "string" && message.slice(0, 17) === "CLARA.IO DOWNLOAD") {
                ApplicationInterface.addAssetToWorldFromURL(message.slice(18));
            }
        }
    }

    Item {
        id: loader
        objectName: "loader";
        property string source: "";
        property var item: null;

        height: pane.scrollHeight
        width: pane.contentWidth

        // this might be looking not clear from the first look
        // but loader.parent is not tabletRoot and it can be null!
        // unfortunately we can't use conditional bindings here due to https://bugreports.qt.io/browse/QTBUG-22005

        onParentChanged: {
            if (parent) {
                anchors.left = Qt.binding(function() { return parent.left })
                anchors.top = Qt.binding(function() { return parent.top })
            } else {
                anchors.left = undefined
                anchors.top = undefined
            }
        }

        signal loaded;
        
        onWidthChanged: {
            if (loader.item) {
                loader.item.width = loader.width;
            }
        }
        
        onHeightChanged: {
            if (loader.item) {
                loader.item.height = loader.height;
            }
        }
        
        function load(newSource, callback) {
            if (loader.item) {
                loader.item.destroy();
                loader.item = null;
            }
            
            QmlSurface.load(newSource, loader, function(newItem) {
                loader.item = newItem;
                loader.item.width = loader.width;
                loader.item.height = loader.height;
                loader.loaded();
                if (loader.item.hasOwnProperty("sendToScript")) {
                    loader.item.sendToScript.connect(tabletRoot.sendToScript);
                }
                if (loader.item.hasOwnProperty("setRootMenu")) {
                    loader.item.setRootMenu(tabletRoot.rootMenu, tabletRoot.subMenu);
                }
                loader.item.forceActiveFocus();
                
                if (callback) {
                    callback();
                }                

                var type = "Unknown";
                if (newSource === "") {
                    type = "Closed";
                } else if (newSource === "hifi/tablet/TabletMenu.qml") {
                    type = "Menu";
                } else if (newSource === "hifi/tablet/TabletHome.qml") {
                    type = "Home";
                } else if (newSource === "hifi/tablet/TabletWebView.qml") {
                    // Handled in `callback()`
                    return;
                } else if (newSource.toLowerCase().indexOf(".qml") > -1) {
                    type = "QML";
                } else {
                    console.log("newSource is of unknown type!");
                }
                
                screenChanged(type, newSource);
            });
        }
    }


    implicitWidth: 480
    implicitHeight: 706
}
