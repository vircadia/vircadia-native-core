import QtQuick 2.5
import controlsUit 1.0
import stylesUit 1.0
import "../windows"
import "../."

Rectangle {
    id: root
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.5);
    visible: false
    property var permissionsOptions: ({'securityOrigin':'none','feature': 'none'})
    property string securityOrigin: 'none'
    property string feature: 'none'
    signal sendPermission(string securityOrigin, string feature, bool shouldGivePermission)

    onFeatureChanged: {
        permissionPopupItem.currentRequestedPermission = feature;
    }

    PermissionPopup {
        id: permissionPopupItem
        onPermissionButtonPressed: {
            if (buttonNumber === 0) {
                root.sendPermission(securityOrigin, feature, true)
            } else {
                root.sendPermission(securityOrigin, feature, false)
            }
            root.visible = false;
            securityOrigin = 'none';
            feature = 'none';
        }
    }
}
