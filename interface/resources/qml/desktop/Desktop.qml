//
//  Desktop.qml
//
//  Created by Bradley Austin Davis on 15 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "../dialogs"
import "../menus"
import "../js/Utils.js" as Utils

// This is our primary 'desktop' object to which all VR dialogs and windows are childed.
FocusScope {
    id: desktop
    objectName: "desktop"
    anchors.fill: parent

    onHeightChanged: d.repositionAll();
    onWidthChanged: d.repositionAll();

    // Controls and windows can trigger this signal to ensure the desktop becomes visible
    // when they're opened.
    signal showDesktop();

    // Allows QML/JS to find the desktop through the parent chain
    property bool desktopRoot: true

    // The VR version of the primary menu
    property var rootMenu: Menu { objectName: "rootMenu" }

    // FIXME: Alpha gradients display as fuschia under QtQuick 2.5 on OSX/AMD
    //        because shaders are 4.2, and do not include #version declarations.
    property bool gradientsSupported: Qt.platform.os != "osx" && !~GL.vendor.indexOf("ATI")

    readonly property alias zLevels: zLevels
    QtObject {
        id: zLevels;
        readonly property real normal: 1 // make windows always appear higher than QML overlays and other non-window controls.
        readonly property real top: 2000
        readonly property real modal: 4000
        readonly property real menu: 8000
    }

    QtObject {
        id: d

        function findChild(item, name) {
            for (var i = 0; i < item.children.length; ++i) {
                if (item.children[i].objectName === name) {
                    return item.children[i];
                }
            }
            return null;
        }

        function findParentMatching(item, predicate) {
            while (item) {
                if (predicate(item)) {
                    break;
                }
                item = item.parent;
            }
            return item;
        }

        function isTopLevelWindow(item) {
            return item.topLevelWindow;
        }

        function isAlwaysOnTopWindow(window) {
            return window.alwaysOnTop;
        }

        function isModalWindow(window) {
            return window.modality !== Qt.NonModal;
        }

        function getTopLevelWindows(predicate) {
            var currentWindows = [];
            if (!desktop) {
                console.log("Could not find desktop for " + item)
                return currentWindows;
            }

            for (var i = 0; i < desktop.children.length; ++i) {
                var child = desktop.children[i];
                if (isTopLevelWindow(child) && (!predicate || predicate(child))) {
                    currentWindows.push(child)
                }
            }
            return currentWindows;
        }

        function getDesktopWindow(item) {
            return findParentMatching(item, isTopLevelWindow)
        }

        function fixupZOrder(windows, basis, topWindow) {
            windows.sort(function(a, b){ return a.z - b.z; });

            if ((topWindow.z >= basis)  &&  (windows[windows.length - 1] === topWindow)) {
                return;
            }

            var lastZ = -1;
            var lastTargetZ = basis - 1;
            for (var i = 0; i < windows.length; ++i) {
                var window = windows[i];
                if (!window.visible) {
                    continue
                }

                if (topWindow && (topWindow === window)) {
                    continue
                }

                if (window.z > lastZ) {
                    lastZ = window.z;
                    ++lastTargetZ;
                }
                if (DebugQML) {
                    console.log("Assigning z order " + lastTargetZ + " to " + window)
                }

                window.z = lastTargetZ;
            }
            if (topWindow) {
                ++lastTargetZ;
                if (DebugQML) {
                    console.log("Assigning z order " + lastTargetZ + " to " + topWindow)
                }
                topWindow.z = lastTargetZ;
            }

            return lastTargetZ;
        }

        function raiseWindow(targetWindow) {
            var predicate;
            var zBasis;
            if (isModalWindow(targetWindow)) {
                predicate = isModalWindow;
                zBasis = zLevels.modal
            } else if (isAlwaysOnTopWindow(targetWindow)) {
                predicate = function(window) {
                    return (isAlwaysOnTopWindow(window) && !isModalWindow(window));
                }
                zBasis = zLevels.top
            } else {
                predicate = function(window) {
                    return (!isAlwaysOnTopWindow(window) && !isModalWindow(window));
                }
                zBasis = zLevels.normal
            }

            var windows = getTopLevelWindows(predicate);
            fixupZOrder(windows, zBasis, targetWindow);
        }

        Component.onCompleted: {
            //offscreenWindow.activeFocusItemChanged.connect(onWindowFocusChanged);
            focusHack.start();
        }

        function onWindowFocusChanged() {
            //console.log("Focus item is " + offscreenWindow.activeFocusItem);

            // FIXME this needs more testing before it can go into production
            // and I already cant produce any way to have a modal dialog lose focus
            // to a non-modal one.
            /*
            var focusedWindow = getDesktopWindow(offscreenWindow.activeFocusItem);

            if (isModalWindow(focusedWindow)) {
                return;
            }

            // new focused window is not modal... check if there are any modal windows
            var windows = getTopLevelWindows(isModalWindow);
            if (0 === windows.length) {
                return;
            }

            // There are modal windows present, force focus back to the top-most modal window
            windows.sort(function(a, b){ return a.z - b.z; });
            windows[windows.length - 1].focus = true;
            */

//            var focusedItem = offscreenWindow.activeFocusItem ;
//            if (DebugQML && focusedItem) {
//                var rect = desktop.mapFromItem(focusedItem, 0, 0, focusedItem.width, focusedItem.height);
//                focusDebugger.x = rect.x;
//                focusDebugger.y = rect.y;
//                focusDebugger.width = rect.width
//                focusDebugger.height = rect.height
//            }
        }


        function repositionAll() {
            var windows = d.getTopLevelWindows();
            for (var i = 0; i < windows.length; ++i) {
                reposition(windows[i]);
            }
        }
    }

    function raise(item) {
        var targetWindow = d.getDesktopWindow(item);
        if (!targetWindow) {
            console.warn("Could not find top level window for " + item);
            return;
        }

        // Fix up the Z-order (takes into account if this is a modal window)
        d.raiseWindow(targetWindow);
        var setFocus = true;
        if (!d.isModalWindow(targetWindow)) {
            var modalWindows = d.getTopLevelWindows(d.isModalWindow);
            if (modalWindows.length) {
                setFocus = false;
            }
        }

        if (setFocus) {
            targetWindow.focus = true;
        }

        reposition(targetWindow);

        showDesktop();
    }

    function reposition(item) {
        if (desktop.width === 0 || desktop.height === 0) {
            return;
        }

        var targetWindow = d.getDesktopWindow(item);
        if (!targetWindow) {
            console.warn("Could not find top level window for " + item);
            return;
        }

        var newPosition = Qt.vector2d(targetWindow.x, targetWindow.y);
        // If the window is completely offscreen, reposition it
        if ((targetWindow.x > desktop.width || (targetWindow.x + targetWindow.width)  < 0) ||
            (targetWindow.y > desktop.height || (targetWindow.y + targetWindow.height)  < 0))  {
            newPosition.x = -1
            newPosition.y = -1
        }

        if (newPosition.x === -1 && newPosition.y === -1) {
            // Set initial window position
            // var minPosition = Qt.vector2d(-windowRect.x, -windowRect.y);
            // var maxPosition = Qt.vector2d(desktop.width - windowRect.width, desktop.height - windowRect.height);
            // newPosition = Utils.clampVector(newPosition, minPosition, maxPosition);
            // newPosition = Utils.randomPosition(minPosition, maxPosition);
            newPosition = Qt.vector2d(desktop.width / 2 - targetWindow.width / 2,
                                      desktop.height / 2 - targetWindow.height / 2);
        }
        targetWindow.x = newPosition.x;
        targetWindow.y = newPosition.y;
    }

    Component { id: messageDialogBuilder; MessageDialog { } }
    function messageBox(properties) {
        return messageDialogBuilder.createObject(desktop, properties);
    }

    Component { id: inputDialogBuilder; QueryDialog { } }
    function inputDialog(properties) {
        return inputDialogBuilder.createObject(desktop, properties);
    }

    Component { id: fileDialogBuilder; FileDialog { } }
    function fileDialog(properties) {
        return fileDialogBuilder.createObject(desktop, properties);
    }

    MenuMouseHandler { id: menuPopperUpper }
    function popupMenu(point) {
        menuPopperUpper.popup(desktop, rootMenu.items, point);
    }

    function toggleMenu(point) {
        menuPopperUpper.toggle(desktop, rootMenu.items, point);
    }

    Keys.onEscapePressed: {
        if (menuPopperUpper.closeLastMenu()) {
            event.accepted = true;
            return;
        }
        event.accepted = false;
    }

    Keys.onLeftPressed: {
        if (menuPopperUpper.closeLastMenu()) {
            event.accepted = true;
            return;
        }
        event.accepted = false;
    }


    function unfocusWindows() {
        var windows = d.getTopLevelWindows();
        for (var i = 0; i < windows.length; ++i) {
            windows[i].focus = false;
        }
        desktop.focus = true;
    }

    FocusHack { id: focusHack; }

    Rectangle {
        id: focusDebugger;
        objectName: "focusDebugger"
        z: 9999; visible: false; color: "red"
        ColorAnimation on color { from: "#7fffff00"; to: "#7f0000ff"; duration: 1000; loops: 9999 }
    }

    Action {
        text: "Toggle Focus Debugger"
        shortcut: "Ctrl+Shift+F"
        enabled: DebugQML
        onTriggered: focusDebugger.visible = !focusDebugger.visible
    }
    
}
