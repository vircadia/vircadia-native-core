import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs;

import "dialogs"

// This is our primary 'desktop' object to which all VR dialogs and
// windows will be childed.
Item {
    id: desktop
    anchors.fill: parent;

    // Debugging help for figuring out focus issues
    property var offscreenWindow;
    onOffscreenWindowChanged: offscreenWindow.activeFocusItemChanged.connect(onWindowFocusChanged);
    function onWindowFocusChanged() { console.log("Focus item is " + offscreenWindow.activeFocusItem); }

    // Allows QML/JS to find the desktop through the parent chain
    property bool desktopRoot: true

    // The VR version of the primary menu
    property var rootMenu: Menu { objectName: "rootMenu" }

    // The tool window, one instance
    property alias toolWindow: toolWindow
    ToolWindow { id: toolWindow }

    // FIXME support always on top flags
    function raise(item) {
        d.raiseWindow(item);
    }

    Component {
        id: messageDialogBuilder
        MessageDialog { }
    }

    Component {
        id: nativeMessageDialogBuilder
        OriginalDialogs.MessageDialog { }
    }

    function messageBox(properties) {
        // Debugging: native message dialog for comparison
        // nativeMessageDialogBuilder.createObject(desktop, properties);
        return messageDialogBuilder.createObject(desktop, properties);
    }

    QtObject {
        id: d

        readonly property int zBasisNormal: 0
        readonly property int zBasisAlwaysOnTop: 4096
        readonly property int zBasisModal: 8192


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
            windows.sort(function(a, b){
                return a.z - b.z;
            });

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

        function raiseWindow(item) {
            var targetWindow = getDesktopWindow(item);
            if (!targetWindow) {
                console.warn("Could not find top level window for " + item);
                return;
            }

            if (!desktop) {
                console.warn("Could not find desktop for window " + targetWindow);
                return;
            }

            var predicate;
            var zBasis;
            if (isModalWindow(targetWindow)) {
                predicate = isModalWindow;
                zBasis = zBasisModal
            } else if (isAlwaysOnTopWindow(targetWindow)) {
                predicate = function(window) {
                    return (isAlwaysOnTopWindow(window) && !isModalWindow(window));
                }
                zBasis = zBasisAlwaysOnTop
            } else {
                predicate = function(window) {
                    return (!isAlwaysOnTopWindow(window) && !isModalWindow(window));
                }
                zBasis = zBasisNormal
            }

            var windows = getTopLevelWindows(predicate);
            fixupZOrder(windows, zBasis, targetWindow);
        }



        //function findMenuChild(menu, childName) {
        //    if (!menu) {
        //        return null;
        //    }

        //    if (menu.type !== 2) {
        //        console.warn("Tried to find child of a non-menu");
        //        return null;
        //    }

        //    var items = menu.items;
        //    var count = items.length;
        //    for (var i = 0; i < count; ++i) {
        //        var child = items[i];
        //        var name;
        //        switch (child.type) {
        //            case 2:
        //                name = child.title;
        //                break;
        //            case 1:
        //                name = child.text;
        //                break;
        //            default:
        //                break;
        //        }

        //        if (name && name === childName) {
        //            return child;
        //        }
        //    }
        //}

        //function findMenu(rootMenu, path) {
        //    if ('string' === typeof(path)) {
        //        path = [ path ]
        //    }

        //    var currentMenu = rootMenu;
        //    for (var i = 0; currentMenu && i < path.length; ++i) {
        //        currentMenu = findMenuChild(currentMenu, path[i]);
        //    }

        //    return currentMenu;
        //}

        //function findInRootMenu(item, path) {
        //    return findMenu(findRootMenu(item), path);
        //}

        //function menuItemsToListModel(parent, items) {
        //    var newListModel = Qt.createQmlObject('import QtQuick 2.5; ListModel {}', parent);
        //    for (var i = 0; i < items.length; ++i) {
        //        var item = items[i];
        //        switch (item.type) {
        //        case 2:
        //            newListModel.append({"type":item.type, "name": item.title, "item": item})
        //            break;
        //        case 1:
        //            newListModel.append({"type":item.type, "name": item.text, "item": item})
        //            break;
        //        case 0:
        //            newListModel.append({"type":item.type, "name": "-----", "item": item})
        //            break;
        //        }
        //    }
        //    return newListModel;
        //}
    }
}



