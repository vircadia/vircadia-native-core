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
        print("INTERACTIVEWINDOWQML SOURCE CHANGED");
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

    function updateInteractiveWindowPositionForMode() {
        print("INTERACTIVEWINDOWQML UPDATING WINDOW POSITION FOR MODE: PRESENTATION MODE IS ", presentationMode);
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            print("INTERACTIVEWINDOWQML PRESENTATION MODE IS VIRTUAL");
            x = interactiveWindowPosition.x;
            y = interactiveWindowPosition.y;
        } else if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            print("INTERACTIVEWINDOWQML PRESENTATION MODE IS NATIVE");
            if (interactiveWindowPosition.x === 0 && interactiveWindowPosition.y === 0) {
                print("INTERACTIVEWINDOWQML NATIVE WINDOW IS AT x:0 AND y:0. CALCULATING NEW POSITION.");
                // default position for native window in center of main application window
                nativeWindow.x = Math.floor(Window.x + (Window.innerWidth / 2) - (interactiveWindowSize.width / 2));
                nativeWindow.y = Math.floor(Window.y + (Window.innerHeight / 2) - (interactiveWindowSize.height / 2));
            } else {
                print("INTERACTIVEWINDOWQML NATIVE WINDOW IS NOT AT x:0 AND y:0. MOVING TO POSITION OF INTERACTIVE WINDOW");
                nativeWindow.x = interactiveWindowPosition.x;
                nativeWindow.y = interactiveWindowPosition.y;
            }
        }
    }

    function updateInteractiveWindowSizeForMode() {
        print("INTERACTIVEWINDOWQML UPDATING SIZE FOR MODE. SETTING ROOT AND CONTENT HOLDER WIDTH AND HEIGHT TO MATCH INTERACTIVE WINDOW.");
        root.width = interactiveWindowSize.width;
        root.height = interactiveWindowSize.height;
        contentHolder.width = interactiveWindowSize.width;
        contentHolder.height = interactiveWindowSize.height;

        if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            print("INTERACTIVEWINDOWQML PRESENTATION MODE IS NATIVE SO ALSO SETTING NATIVE WINDOW WIDTH AND HEIGHT TO MATCH INTERACTIVE WINDOW.");
            nativeWindow.width = interactiveWindowSize.width;
            nativeWindow.height = interactiveWindowSize.height;
        }
    }

    function updateContentParent() {
        print("INTERACTIVEWINDOWQML UPDATE CONTENT PARENT");
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            print("INTERACTIVEWINDOWQML PRESENTATION MODE IS VIRTUAL");
            contentHolder.parent = root;
        } else if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow) {
            print("INTERACTIVEWINDOWQML PRESENTATION MODE IS NATIVE");
            contentHolder.parent = nativeWindow.contentItem;
        }
    }

    function setupPresentationMode() {
        print("INTERACTIVEWINDOWQML SET UP PRESENTATION MODE");
        if (presentationMode === Desktop.PresentationMode.VIRTUAL) {
            print("INTERACTIVEWINDOWQML PRESENTATION MODE IS VIRTUAL. UPDATING CONTENT PARENT, UPDATING INTERACTIVE WINDOW POSITION FOR MODE, AND SETTING VIRTUAL WINDOW VISIBILITY TO MATCH INTERACTIVE WINDOW VISIBILITY.");
            if (nativeWindow) {
                print("INTERACTIVEWINDOWQML SETTING NATIVE WINDOW TO NOT VISIBLE");
                nativeWindow.setVisible(false);
            }
            updateContentParent();
            updateInteractiveWindowPositionForMode();
            shown = interactiveWindowVisible;
        } else if (presentationMode === Desktop.PresentationMode.NATIVE) {
            print("INTERACTIVEWINDOWQML PRESENTATION MODE IS NATIVE");
            shown = false;
            if (nativeWindow) {
                print("INTERACTIVEWINDOWQML NATIVE EXISTS. UPDATING CONTENT PARENT, UPDATING INTERACTIVE WINDOW POSITION FOR MODE, AND SETTING NATIVE WINDOW VISIBILITY TO MATCH INTERACTIVE WINDOW VISIBILITY.");
                updateContentParent();
                updateInteractiveWindowPositionForMode();
                nativeWindow.setVisible(interactiveWindowVisible);
            }
        } else if (presentationMode === modeNotSet) {
            console.error("presentationMode should be set.");
        }
    }

    Component.onCompleted: {
        print("INTERACTIVEWINDOWQML ONROOTCOMPLETED FIX FOR OSX PARENT LOSS!!!!! CONNECTING SIGNALS TO UPDATE CONTENT PARENT WHEN PARENT HEIGHT OR WIDTH CHANGES. SETTING X,Y,WIDTH, AND HEIGHT TO MATCH INTERACTIVE WINDOW. CREATING NATIVE WINDOW. NATIVE WINDOW IS ALWAYS ON TOP FOR NON-WINDOWS.");
        // Fix for parent loss on OSX:
        parent.heightChanged.connect(updateContentParent);
        parent.widthChanged.connect(updateContentParent);

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
                Component.onCompleted: {
                    print("INTERACTIVEWINDOWQML NATIVE WINDOW COMPLETED");
                }

                Rectangle {
                    color: hifi.colors.baseGray
                    anchors.fill: parent
                    
                    MouseArea {
                        id: helpButtonMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            Tablet.playSound(TabletEnums.ButtonHover);
                        }
                        onClicked: {
                            nativeWindow.x = 0;
                            nativeWindow.y = 0;
                        }
                    }
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
                print("NATIVE WINDOW X CHANGED SIGNAL. THE WINDOW WAS NATIVE AND VISIBLE");
                interactiveWindowPosition = Qt.point(nativeWindow.x, interactiveWindowPosition.y);
            }
        });
        
        nativeWindow.yChanged.connect(function() {
            
            if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow.visible) {
                print("NATIVE WINDOW Y CHANGED SIGNAL. THE WINDOW WAS NATIVE AND VISIBLE");
                interactiveWindowPosition = Qt.point(interactiveWindowPosition.x, nativeWindow.y);
            }
        });

        nativeWindow.widthChanged.connect(function() {
            if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow.visible) {
                print("NATIVE WINDOW WIDTH CHANGED SIGNAL. THE WINDOW WAS NATIVE AND VISIBLE");
                interactiveWindowSize = Qt.size(nativeWindow.width, interactiveWindowSize.height);
            }
        });
        
        nativeWindow.heightChanged.connect(function() {
            if (presentationMode === Desktop.PresentationMode.NATIVE && nativeWindow.visible) {
                print("NATIVE WINDOW HEIGHT CHANGED SIGNAL. THE WINDOW WAS NATIVE AND VISIBLE");
                interactiveWindowSize = Qt.size(interactiveWindowSize.width, nativeWindow.height);
            }
        });

        nativeWindow.closing.connect(function(closeEvent) {
            print("NATIVE WINDOW CLOSING SIGNAL.");
            closeEvent.accepted = false;
            windowClosed();
        });

        // finally set the initial window mode:
        setupPresentationMode();

        initialized = true;
    }

    Component.onDestruction: {
        parent.heightChanged.disconnect(updateContentParent);
        parent.widthChanged.disconnect(updateContentParent);
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
    function onRequestNewHeight(newWidth) {
        interactiveWindowSize.width = newWidth;
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
            print("VIRTUAL WINDOW Y CHANGE");
            interactiveWindowPosition = Qt.point(interactiveWindowPosition.x, y);
        } else {
            print("NATIVE WINDOW Y CHANGE");
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
