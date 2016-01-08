var OFFSCREEN_ROOT_OBJECT_NAME = "desktopRoot"
var OFFSCREEN_WINDOW_OBJECT_NAME = "topLevelWindow"

function findChild(item, name) {
    for (var i = 0; i < item.children.length; ++i) {
        if (item.children[i].objectName === name) {
            return item.children[i];
        }
    }
    return null;
}

function findParent(item, name) {
    while (item) {
        if (item.objectName === name) {
            return item;
        }
        item = item.parent;
    }
    return null;
}

function getDesktop(item) {
    return findParent(item, OFFSCREEN_ROOT_OBJECT_NAME);
}

function findRootMenu(item) {
    item = getDesktop(item);
    return item ? item.rootMenu : null;
}


function getTopLevelWindows(item) {
    var desktop = getDesktop(item);
    var currentWindows = [];
    if (!desktop) {
        console.log("Could not find desktop for " + item)
        return currentWindows;
    }

    for (var i = 0; i < desktop.children.length; ++i) {
        var child = desktop.children[i];
        if (Global.OFFSCREEN_WINDOW_OBJECT_NAME === child.objectName) {
            var windowId = child.toString();
            currentWindows.push(child)
        }
    }
    return currentWindows;
}


function getDesktopWindow(item) {
    item = findParent(item, OFFSCREEN_WINDOW_OBJECT_NAME);
    return item;
}

function closeWindow(item) {
    item = findDialog(item);
    if (item) {
        item.enabled = false
    } else {
        console.warn("Could not find top level dialog")
    }
}

function findMenuChild(menu, childName) {
    if (!menu) {
        return null;
    }

    if (menu.type !== 2) {
        console.warn("Tried to find child of a non-menu");
        return null;
    }

    var items = menu.items;
    var count = items.length;
    for (var i = 0; i < count; ++i) {
        var child = items[i];
        var name;
        switch (child.type) {
            case 2:
                name = child.title;
                break;
            case 1:
                name = child.text;
                break;
            default:
                break;
        }

        if (name && name === childName) {
            return child;
        }
    }
}

function findMenu(rootMenu, path) {
    if ('string' === typeof(path)) {
        path = [ path ]
    }

    var currentMenu = rootMenu;
    for (var i = 0; currentMenu && i < path.length; ++i) {
        currentMenu = findMenuChild(currentMenu, path[i]);
    }

    return currentMenu;
}

function findInRootMenu(item, path) {
    return findMenu(findRootMenu(item), path);
}


function menuItemsToListModel(parent, items) {
    var newListModel = Qt.createQmlObject('import QtQuick 2.5; ListModel {}', parent);
    for (var i = 0; i < items.length; ++i) {
        var item = items[i];
        switch (item.type) {
        case 2:
            newListModel.append({"type":item.type, "name": item.title, "item": item})
            break;
        case 1:
            newListModel.append({"type":item.type, "name": item.text, "item": item})
            break;
        case 0:
            newListModel.append({"type":item.type, "name": "-----", "item": item})
            break;
        }
    }
    return newListModel;
}

function raiseWindow(item) {
    var targetWindow = getDesktopWindow(item);
    if (!targetWindow) {
        console.warn("Could not find top level window for " + item);
        return;
    }
    
    var desktop = getDesktop(targetWindow);
    if (!desktop) {
        //console.warn("Could not find desktop for window " + targetWindow);
        return;
    }
        
    var maxZ = 0;
    var minZ = 100000;
    var windows = desktop.windows;
    windows.sort(function(a, b){
       return a.z - b.z; 
    });
    var lastZ = -1;
    var lastTargetZ = -1;
    for (var i = 0; i < windows.length; ++i) {
        var window = windows[i];
        if (window.z > lastZ) {
            lastZ = window.z;
            ++lastTargetZ;
        }
        window.z = lastTargetZ;
    }
    targetWindow.z = lastTargetZ + 1;
}
