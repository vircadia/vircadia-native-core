import QtQuick 2.9

Item {
    width: 92
    height: 92

    property int wearablesCount: 0
    onWearablesCountChanged: {
        console.debug('AvatarThumbnail: wearablesCount = ', wearablesCount)
    }

    property alias dropShadowRadius: avatarImage.dropShadowRadius
    property alias dropShadowHorizontalOffset: avatarImage.dropShadowHorizontalOffset
    property alias dropShadowVerticalOffset: avatarImage.dropShadowVerticalOffset

    property alias imageUrl: avatarImage.source

    ShadowImage {
        id: avatarImage
        anchors.fill: parent
    }

    AvatarWearablesIndicator {
        anchors.left: avatarImage.left
        anchors.bottom: avatarImage.bottom
        anchors.leftMargin: 57
        wearablesCount: parent.wearablesCount
        visible: parent.wearablesCount !== 0
    }
}
