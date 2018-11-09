import QtQuick 2.9
import controlsUit 1.0
import stylesUit 1.0

ShadowRectangle {
    property int wearablesCount: 0

    dropShadowRadius: 4
    dropShadowHorizontalOffset: 0
    dropShadowVerticalOffset: 0

    width: 46.5
    height: 46.5
    radius: width / 2

    AvatarAppStyle {
        id: style
    }

    color: style.colors.greenHighlight

    HiFiGlyphs {
        width: 26.5
        height: 13.8
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
        text: "\ue02e"
    }

    Item {
        width: 46.57
        height: 23
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2.76

        // TextStyle2
        RalewayBold {
            size: 15;
            anchors.horizontalCenter: parent.horizontalCenter
            text: wearablesCount
        }
    }
}
