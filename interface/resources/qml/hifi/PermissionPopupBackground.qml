import QtQuick 2.5
import controlsUit 1.0
import stylesUit 1.0
import "../windows"
import "../."

Rectangle {
    id: permissionPopupBackground
    anchors.fill: parent
    color: QT.rgba(0, 0, 0, 0.7);
    HifiConstants { id: hifi }

    PermissionPopup {
        id: permissionPopupItem
        visible: false
        Component.onCompleted: {
            console.log("\n\n TESTING!! \n\n")
        }
    }

    onFeaturePermissionRequested: {
        permissionPopupItem.onLeftButtonClicked = function() {
            console.log("TEST FOR PERMISSIONS :: LEFT CLICKED");
            grantFeaturePermission(securityOrigin, feature, true);
                .visible = false;
        }
        permissionPopupItem.onRightButtonClicked = function() {
            console.log("TEST FOR PERMISSIONS :: RIGHT CLICKED");
            grantFeaturePermission(securityOrigin, feature, false);
            permissionPopupBackground.visible = false;
        }
        permissionPopupBackground.visible = true;
    }
}
