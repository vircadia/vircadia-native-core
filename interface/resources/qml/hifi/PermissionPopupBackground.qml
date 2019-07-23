import QtQuick 2.5
import controlsUit 1.0
import stylesUit 1.0
import "../windows"
import "../."

Rectangle {
    id: permissionPopupBackground
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.5);
    HifiConstants { id: hifi }
    visible: true
    property variant permissionsOptions: {'securityOrigin':'none','feature':'none'}
    signal sendPermission(string securityOrigin, string feature, bool shouldGivePermission)
 
    Component.onCompleted: {
        console.log("loaded component");
        // console.log("\n\n TESTING!! \n\n")
    }

    PermissionPopup {
        id: permissionPopupItem
        Component.onCompleted: {
            console.log("test");
            // console.log("\n\n TESTING!! \n\n")
        }
        onPermissionButtonPressed: {
            console.log("JUST MADE IT TO ON PERMISSIONS PRESSEED!");
            console.log(buttonNumber);
            if (buttonNumber === 0) {
                permissionPopupBackground.sendPermission(permissionsOptions.securityOrigin, permissionsOptions.feature, true)
            } else {
                permissionPopupBackground.sendPermission(permissionsOptions.securityOrigin, permissionsOptions.feature, false)
            }
            permissionPopupBackground.visible = false;
            permissionsOptions.securityOrigin = "none";
            permissionsOptions.feature = "none";
        }
    }
}
