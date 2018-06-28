//
//  InteractiveWindow.qml
//
//  Created by Thijs Wenker on 2018-06-25
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.3
import InteractiveWindowFlags 1.0

import "windows" as Windows
import "controls"
import "controls-uit" as Controls
import "styles"
import "styles-uit"

Windows.Window {
    id: root;
    HifiConstants { id: hifi }
    title: "InteractiveWindow";
    resizable: true;
    // Virtual window visibility
    shown: false;
    focus: true;
    property var channel;
    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false;

    property var flags: 0;

    property var source;
    property var dynamicContent;
    property var nativeWindow;

    // custom visibility flag for interactiveWindow to proxy virtualWindow.shown / nativeWindow.visible
    property var interactiveWindowVisible: true;

    property point interactiveWindowPosition;

    property size interactiveWindowSize;

    // Keyboard control properties in case needed by QML content.
    property bool keyboardEnabled: false;
    property bool keyboardRaised: false;
    property bool punctuationMode: false;

    readonly property int modeNotSet: 0;
    readonly property int modeNative: 1;
    readonly property int modeVirtual: 2;

    property int windowMode: modeNotSet;

    property bool forceNative: false;
    property bool forceVirtual: false;

    property string windowModeText: getModeString();

    function getModeString() {
        switch (windowMode) {
            case modeNotSet:
                return "none";
            case modeNative:
                return "native";
            case modeVirtual:
                return "virtual";
        }
        return "unknown";
    }

    onWindowModeChanged: {
        windowModeText = getModeString();
    }

    onSourceChanged: {
        if (dynamicContent) {
            dynamicContent.destroy();
            dynamicContent = null; 
        }
        QmlSurface.load(source, contentHolder, function(newObject) {
            dynamicContent = newObject;
            if (dynamicContent && dynamicContent.anchors) {
                dynamicContent.anchors.fill = contentHolder;
            }
        });
    }

    function updateInteractiveWindowPositionForMode() {
        if (windowMode === modeVirtual) {
            x = interactiveWindowPosition.x;
            y = interactiveWindowPosition.y;
        } else if (windowMode === modeVirtual && nativeWindow) {
            nativeWindow.x = interactiveWindowPosition.x;
            nativeWindow.y = interactiveWindowPosition.y;
        }
    }

    function updateInteractiveWindowSizeForMode() {
        if (windowMode === modeVirtual) {
            width = interactiveWindowSize.width;
            height = interactiveWindowSize.height;
        } else if (windowMode === modeVirtual && nativeWindow) {
            nativeWindow.width = interactiveWindowSize.width;
            nativeWindow.height = interactiveWindowSize.heigth;
        }
    }

    function trySwitchWindowMode() {
        if (windowMode !== modeVirtual && (HMD.active || (forceVirtual && !forceNative))) {
            windowMode = modeVirtual;
            if (nativeWindow) {
                nativeWindow.setVisible(false);
            }
            contentHolder.parent = root;
            updateInteractiveWindowPositionForMode();
            shown = interactiveWindowVisible;
        } else if (windowMode !== modeNative && (!HMD.active || (forceNative && !forceVirtual))) {
            windowMode = modeNative;
            shown = false;
            if (nativeWindow) {
                contentHolder.parent = nativeWindow.contentItem;
                nativeWindow.setVisible(interactiveWindowVisible);
                updateInteractiveWindowPositionForMode();
            }
        } else if (windowMode === modeNotSet) {
            console.error("windowMode should be set.");
        }
    }

    function displayModeChanged(isHMD) {
        trySwitchWindowMode();
    }
    
    Component.onCompleted: {
        HMD.displayModeChanged.connect(displayModeChanged);

        forceVirtual = (flags & InteractiveWindowFlags.ForceVirtual) === InteractiveWindowFlags.ForceVirtual;
        forceNative = (flags & InteractiveWindowFlags.ForceNative) === InteractiveWindowFlags.ForceNative;

        x = interactiveWindowPosition.x;
        y = interactiveWindowPosition.y;
        width = interactiveWindowSize.width;
        height = interactiveWindowSize.height;

        if (!forceVirtual || (forceVirtual && forceNative)) {
            nativeWindow = Qt.createQmlObject('
                import QtQuick 2.3;
                import QtQuick.Window 2.3;

                Window {
                    id: root;
                    Rectangle {
                        color: hifi.colors.baseGray
                        anchors.fill: parent
                    }
                }', root, 'InteractiveWindow.qml->nativeWindow');
            nativeWindow.title = root.title;
            var nativeWindowFlags = Qt.Window |
                Qt.WindowTitleHint |
                Qt.WindowSystemMenuHint |
                Qt.WindowCloseButtonHint |
                Qt.WindowMaximizeButtonHint |
                Qt.WindowMinimizeButtonHint;
            if ((flags & InteractiveWindowFlags.AlwaysOnTop) === InteractiveWindowFlags.AlwaysOnTop) {
                nativeWindowFlags |= Qt.WindowStaysOnTopHint;
            }
            nativeWindow.flags = nativeWindowFlags;

            nativeWindow.x = interactiveWindowPosition.x;
            nativeWindow.y = interactiveWindowPosition.y;

            nativeWindow.width = interactiveWindowSize.width;
            nativeWindow.height = interactiveWindowSize.height;

            nativeWindow.xChanged.connect(function() {
                if (windowMode === modeNative && nativeWindow.visible) {
                    interactiveWindowPosition = Qt.point(nativeWindow.x, interactiveWindowPosition.y);
                }
            });
            nativeWindow.yChanged.connect(function() {
                if (windowMode === modeNative && nativeWindow.visible) {
                    interactiveWindowPosition = Qt.point(interactiveWindowPosition.x, nativeWindow.y);
                }
            });

            nativeWindow.widthChanged.connect(function() {
                if (windowMode === modeNative && nativeWindow.visible) {
                    interactiveWindowSize = Qt.size(nativeWindow.width, interactiveWindowSize.height);
                }
            });
            nativeWindow.heightChanged.connect(function() {
                if (windowMode === modeNative && nativeWindow.visible) {
                    interactiveWindowSize = Qt.size(interactiveWindowSize.width, nativeWindow.height);
                }
            });
        }

        // finally set the initial window mode:
        trySwitchWindowMode();
    }

    // Handle message traffic from the script that launched us to the loaded QML
    function fromScript(message) {
        if (root.dynamicContent && root.dynamicContent.fromScript) {
            root.dynamicContent.fromScript(message);
        }
    }

    function show() {
        interactiveWindowVisible = true;
        raiseWindow();
    }

    function raiseWindow() {
        if (windowMode === modeVirtual) {
            raise();
        } else if (windowMode === modeNative && nativeWindow) {
            nativeWindow.raise();
        }
    }
    
    // Handle message traffic from our loaded QML to the script that launched us
    signal sendToScript(var message);

    onDynamicContentChanged: {
        if (dynamicContent && dynamicContent.sendToScript) {
            dynamicContent.sendToScript.connect(sendToScript);
        }
    }

    onInteractiveWindowVisibleChanged: {
        if (windowMode === modeVirtual) {
            shown = interactiveWindowVisible;
        } else if (windowMode === modeNative && nativeWindow) {
            nativeWindow.setVisible(interactiveWindowVisible);
        }
    }

    onTitleChanged: {
        if (nativeWindow) {
            nativeWindow.title = title;
        }
    }

    onXChanged: {
        if (windowMode === modeVirtual) {
            interactiveWindowPosition = Qt.point(x, interactiveWindowPosition.y);
        }
    }

    onYChanged: {
        if (windowMode === modeVirtual) {
            interactiveWindowPosition = Qt.point(interactiveWindowPosition.x, y);
        }
    }

    onWidthChanged: {
        if (windowMode === modeVirtual) {
            interactiveWindowSize = Qt.size(width, interactiveWindowSize.height);
        }
    }

    onHeightChanged: {
        if (windowMode === modeVirtual) {
            interactiveWindowSize = Qt.size(interactiveWindowSize.width, height);
        }
    }

    onInteractiveWindowPositionChanged: {
        updateInteractiveWindowPositionForMode();
    }

    onInteractiveWindowSizeChanged: {
        updateInteractiveWindowSizeForMode();
    }

    Item {
        id: contentHolder
        anchors.fill: parent
    }
}
