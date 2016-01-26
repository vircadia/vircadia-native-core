import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs;

import "../dialogs"
import "../menus"
import "../js/Utils.js" as Utils

// This is our primary 'desktop' object to which all VR dialogs and
// windows will be childed.
FocusScope {
    id: desktop
    anchors.fill: parent;
    objectName: "desktop"

    // Allows QML/JS to find the desktop through the parent chain
    property bool desktopRoot: true

    // The VR version of the primary menu
    property var rootMenu: Menu { objectName: "rootMenu" }

    readonly property alias zLevels: zLevels
    QtObject {
        id: zLevels;
        readonly property real normal: 0
        readonly property real top: 2000
        readonly property real modal: 4000
        readonly property real menu: 8000
    }

    QtObject {
        id: d
        readonly property var messageDialogBuilder: Component { MessageDialog { } }

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

        function isDesktop(item) {
            return item.desktopRoot;
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
            focus = true;
        }

        reposition(targetWindow);
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

        var windowRect = targetWindow.framedRect();
        var minPosition = Qt.vector2d(-windowRect.x, -windowRect.y);
        var maxPosition = Qt.vector2d(desktop.width - windowRect.width, desktop.height - windowRect.height);
        var newPosition;
        if (targetWindow.x === -1 && targetWindow.y === -1) {
            // Set initial window position
            newPosition = Utils.randomPosition(minPosition, maxPosition);
        } else {
            newPosition = Utils.clampVector(Qt.vector2d(targetWindow.x, targetWindow.y), minPosition, maxPosition);
        }
        targetWindow.x = newPosition.x;
        targetWindow.y = newPosition.y;
    }

    function repositionAll() {
        var windows = d.getTopLevelWindows();
        for (var i = 0; i < windows.length; ++i) {
            reposition(windows[i]);
        }
    }

    onHeightChanged: repositionAll();
    onWidthChanged: repositionAll();

    function messageBox(properties) {
        return messageDialogBuilder.createObject(desktop, properties);
    }

    Component { id: queryDialogBuilder; QueryDialog { } }
    function queryBox(properties) {
        return queryDialogBuilder.createObject(desktop, properties);
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

    // Debugging help for figuring out focus issues
    property var offscreenWindow;
    onOffscreenWindowChanged: {
        offscreenWindow.activeFocusItemChanged.connect(onWindowFocusChanged);
        focusHack.start();
    }

    FocusHack { id: focusHack; }

    function onWindowFocusChanged() {
        console.log("Focus item is " + offscreenWindow.activeFocusItem);
        var focusedItem = offscreenWindow.activeFocusItem ;
        if (DebugQML && focusedItem) {
            var rect = desktop.mapFromItem(focusedItem, 0, 0, focusedItem.width, focusedItem.height);
            focusDebugger.x = rect.x;
            focusDebugger.y = rect.y;
            focusDebugger.width = rect.width
            focusDebugger.height = rect.height
        }
    }

    Rectangle {
        id: focusDebugger;
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



