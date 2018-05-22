import QtQuick 2.9
import QtGraphicalEffects 1.0

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
        visible: status !== Image.Loading
        radius: 5
        fillMode: Image.PreserveAspectCrop
    }

    ShadowRectangle {
        anchors.fill: parent;
        color: 'white'
        visible: avatarImage.status === Image.Loading
        radius: avatarImage.radius
        border.width: avatarImage.border.width
        border.color: avatarImage.border.color

        dropShadowRadius: avatarImage.dropShadowRadius;
        dropShadowHorizontalOffset: avatarImage.dropShadowHorizontalOffset
        dropShadowVerticalOffset: avatarImage.dropShadowVerticalOffset

        Spinner {
            id: spinner
            visible: parent.visible
            anchors.fill: parent;
        }
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
