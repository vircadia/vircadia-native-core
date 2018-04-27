import QtQuick 2.9

Item {
    width: 92
    height: 92
    property alias wearableIndicator: indicator

    property int wearablesCount: 0
    onWearablesCountChanged: {
        console.debug('AvatarThumbnail: wearablesCount = ', wearablesCount)
    }

    property alias dropShadowRadius: avatarImage.dropShadowRadius
    property alias dropShadowHorizontalOffset: avatarImage.dropShadowHorizontalOffset
    property alias dropShadowVerticalOffset: avatarImage.dropShadowVerticalOffset

    property alias imageUrl: avatarImage.source
    property alias border: avatarImage.border

    ShadowImage {
        id: avatarImage
        anchors.fill: parent
        radius: 6
    }

    AvatarWearablesIndicator {
        id: indicator
        anchors.left: avatarImage.left
        anchors.bottom: avatarImage.bottom
        anchors.leftMargin: 57
        wearablesCount: parent.wearablesCount
        visible: parent.wearablesCount !== 0
    }
}
