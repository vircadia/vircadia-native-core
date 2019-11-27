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

import "windows" as Windows
import "controls"
import controlsUit 1.0 as Controls
import "styles"
import stylesUit 1.0

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

    signal selfDestruct();

    property var additionalFlags: 0;
    property var overrideFlags: 0;

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

    property int presentationMode: 0;

    property var initialized: false;
    onSourceChanged: {
        if (dynamicContent) {
            dynamicContent.destroy();
            dynamicContent = null;
        }
        QmlSurface.load(source, contentHolder, function(newObject) {
            dynamicContent = newObject;
            updateInteractiveWindowSizeForMode();
            if (dynamicContent && dynamicContent.anchors) {
                dynamicContent.anchors.fill = contentHolder;
            }
        });
    }
    
    Timer {
        id: timer
        interval: 500;
        repeat: false;
        onTriggered: {
            updateContentParent();
        }
    }

    function updateInteractiveWindowPositionForMode() {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            x = interactiveWindowPosition.x;
            y = interactiveWindowPosition.y;
        } else if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            if (interactiveWindowPosition.x === 0 && interactiveWindowPosition.y === 0) {
                // default position for native window in center of main application window
                nativeWindow.x = Math.floor(Window.x + (Window.innerWidth / 2) - (interactiveWindowSize.width / 2));
                nativeWindow.y = Math.floor(Window.y + (Window.innerHeight / 2) - (interactiveWindowSize.height / 2));
            } else {
                nativeWindow.x = interactiveWindowPosition.x;
                nativeWindow.y = interactiveWindowPosition.y;
            }
        }
    }

    function updateInteractiveWindowSizeForMode() {
        root.width = interactiveWindowSize.width;
        root.height = interactiveWindowSize.height;
        contentHolder.width = interactiveWindowSize.width;
        contentHolder.height = interactiveWindowSize.height;

        if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            nativeWindow.width = interactiveWindowSize.width;
            nativeWindow.height = interactiveWindowSize.height;
        }
    }

    function updateContentParent() {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            contentHolder.parent = root;
        } else if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            contentHolder.parent = nativeWindow.contentItem;
        }
    }

    function setupPresentationMode() {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            if (nativeWindow) {
                nativeWindow.setVisible(false);
            }
            updateInteractiveWindowPositionForMode();
            shown = interactiveWindowVisible;
        } else if (presentationMode === Desktop.PresentationMode.NATIVE) {
            shown = false;
            if (nativeWindow) {
                updateInteractiveWindowPositionForMode();
                nativeWindow.setVisible(interactiveWindowVisible);
            }
        } else if (presentationMode === modeNotSet) {
            console.error("presentationMode should be set.");
        }
    }

    Component.onCompleted: {
        
        x = interactiveWindowPosition.x;
        y = interactiveWindowPosition.y;
        width = interactiveWindowSize.width;
        height = interactiveWindowSize.height;

        nativeWindow = Qt.createQmlObject('
            import QtQuick 2.3;
            import QtQuick.Window 2.3;

            Window {
                id: root;
                width: interactiveWindowSize.width
                height: interactiveWindowSize.height
                // fix for missing content on OSX initial startup with a non-maximized interface window. It seems that in this case, we cannot update
                // the content parent during creation of the Window root. This added delay will update the parent after the root has finished loading.
                Component.onCompleted: {
                    timer.start();
                }

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
        // only use the always on top feature for non Windows OS
        if (Qt.platform.os !== "windows" && (root.additionalFlags & Desktop.ALWAYS_ON_TOP)) {
            nativeWindowFlags |= Qt.WindowStaysOnTopHint;
        }
        nativeWindow.flags = root.overrideFlags || nativeWindowFlags;

        nativeWindow.x = interactiveWindowPosition.x;
        nativeWindow.y = interactiveWindowPosition.y;

        nativeWindow.width = interactiveWindowSize.width;
        nativeWindow.height = interactiveWindowSize.height;

        nativeWindow.xChanged.connect(function() {
            if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow.visible) {
                interactiveWindowPosition = Qt.point(nativeWindow.x, interactiveWindowPosition.y);
            }
        });
        
        nativeWindow.yChanged.connect(function() {
            if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow.visible) {
                interactiveWindowPosition = Qt.point(interactiveWindowPosition.x, nativeWindow.y);
            }
        });

        nativeWindow.widthChanged.connect(function() {
            if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow.visible) {
                interactiveWindowSize = Qt.size(nativeWindow.width, interactiveWindowSize.height);
            }
        });
        
        nativeWindow.heightChanged.connect(function() {
            if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow.visible) {
                interactiveWindowSize = Qt.size(interactiveWindowSize.width, nativeWindow.height);
            }
        });

        nativeWindow.closing.connect(function(closeEvent) {
            closeEvent.accepted = false;
            windowClosed();
        });

        // finally set the initial window mode:
        setupPresentationMode();
        updateContentParent();

        initialized = true;
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
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            raise();
        } else if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            nativeWindow.raise();
        }
    }

    // Handle message traffic from our loaded QML to the script that launched us
    signal sendToScript(var message);

    // Children of this InteractiveWindow Item are able to request a new width and height
    // for the parent Item (this one) and its associated C++ InteractiveWindow using these methods.
    function onRequestNewWidth(newWidth) {
        interactiveWindowSize.width = newWidth;
        updateInteractiveWindowSizeForMode();
    }
    function onRequestNewHeight(newHeight) {
        interactiveWindowSize.height = newHeight;
        updateInteractiveWindowSizeForMode();
    }
    
    // These signals are used to forward key-related events from the QML to the C++.
    signal keyPressEvent(int key, int modifiers);
    signal keyReleaseEvent(int key, int modifiers);

    onDynamicContentChanged: {
        if (dynamicContent && dynamicContent.sendToScript) {
            dynamicContent.sendToScript.connect(sendToScript);
        }

        if (dynamicContent && dynamicContent.requestNewWidth) {
            dynamicContent.requestNewWidth.connect(onRequestNewWidth);
        }

        if (dynamicContent && dynamicContent.requestNewHeight) {
            dynamicContent.requestNewHeight.connect(onRequestNewHeight);
        }

        if (dynamicContent && dynamicContent.keyPressEvent) {
            dynamicContent.keyPressEvent.connect(keyPressEvent);
        }

        if (dynamicContent && dynamicContent.keyReleaseEvent) {
            dynamicContent.keyReleaseEvent.connect(keyReleaseEvent);
        }
    }

    onInteractiveWindowVisibleChanged: {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            shown = interactiveWindowVisible;
        } else if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            if (!nativeWindow.visible && interactiveWindowVisible) {
                nativeWindow.showNormal();
            } else {
                nativeWindow.setVisible(interactiveWindowVisible);
            }
        }
    }

    onTitleChanged: {
        if (nativeWindow) {
            nativeWindow.title = title;
        }
    }

    onXChanged: {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            interactiveWindowPosition = Qt.point(x, interactiveWindowPosition.y);
        }
    }

    onYChanged: {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            interactiveWindowPosition = Qt.point(interactiveWindowPosition.x, y);
        }
    }

    onWidthChanged: {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            interactiveWindowSize = Qt.size(width, interactiveWindowSize.height);
        }
    }

    onHeightChanged: {
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            interactiveWindowSize = Qt.size(interactiveWindowSize.width, height);
        }
    }

    onPresentationModeChanged: {
        if (initialized) {
            setupPresentationMode();
            updateContentParent();
        }
    }

    onWindowClosed: {
        // set invisible on close, to make it not re-appear unintended after switching PresentationMode
        interactiveWindowVisible = false;

        if ((root.additionalFlags & Desktop.CLOSE_BUTTON_HIDES) !== Desktop.CLOSE_BUTTON_HIDES) {
            selfDestruct();
        }
    }

    Item {
        id: contentHolder
        anchors.fill: parent
    }
}
