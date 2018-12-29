import QtQuick 2.0
import "../avatarPackager" 1.0

Item {
    id: root
    width: 480
    height: 706

    AvatarPackagerApp {
        width: parent.width
        height: parent.height

        desktopObject: tabletRoot
    }
}
