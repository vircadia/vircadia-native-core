import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

AnimatedImage {
    id: root

    width: 128
    height: 128

    property bool white: false

    source: white ? "../../../icons/loader-snake-256-wf.gif" : "../../../icons/loader-snake-256.gif"
    playing: true
}
