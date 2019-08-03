import QtQuick 2.5

Rectangle {
    id: root
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.5);
    visible: false
    property string securityOrigin: 'none'
    property string feature: 'none'
    signal sendPermission(string securityOrigin, string feature, bool shouldGivePermission)

    onFeatureChanged: {
        permissionPopupItem.currentRequestedPermission = feature;
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: false
    }

    PermissionPopup {
        id: permissionPopupItem
        onPermissionButtonPressed: {
            if (buttonNumber === 0) {
                root.sendPermission(securityOrigin, feature, false);
            } else {
                root.sendPermission(securityOrigin, feature, true);
            }
            root.visible = false;
            securityOrigin = 'none';
            feature = 'none';
        }
    }
}
