import QtQuick 2.5
import controlsUit 1.0
import stylesUit 1.0
import "../windows"
import "../."

Rectangle {
    id: root
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.5);
    HifiConstants { id: hifi }
    visible: false
    property variant permissionsOptions: {'securityOrigin':'none','feature': -1}
    signal sendPermission(string securityOrigin, int feature, bool shouldGivePermission)
 
    Component.onCompleted: {
        console.log("loaded component");
        // console.log("\n\n TESTING!! \n\n")
    }

    PermissionPopup {
        id: permissionPopupItem
        Component.onCompleted: {
            permissionPopupItem.currentRequestedPermission = permissionsOptions.feature;
            console.log("test");
            // console.log("\n\n TESTING!! \n\n")
        }
        onPermissionButtonPressed: {
            console.log("JUST MADE IT TO ON PERMISSIONS PRESSEED!");
            console.log(buttonNumber);
            if (buttonNumber === 0) {
                root.sendPermission(permissionsOptions.securityOrigin, permissionsOptions.feature, true)
            } else {
                root.sendPermission(permissionsOptions.securityOrigin, permissionsOptions.feature, false)
            }
            root.visible = false;
            permissionsOptions.securityOrigin = "none";
            permissionsOptions.feature = -1;
        }
    }
}
