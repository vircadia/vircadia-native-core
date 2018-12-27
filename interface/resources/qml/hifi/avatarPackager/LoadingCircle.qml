import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

Image {
    id: root

    width: 128
    height: 128

    source: "../../../images/loader-snake-128.png"

    RotationAnimation on rotation {
        duration: 2000
        loops: Animation.Infinite
        from: 0
        to: 360
    }
}