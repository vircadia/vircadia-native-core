import QtQuick 2.5

import "."

Window {
    id: root
    anchors.centerIn: parent
    modality: Qt.ApplicationModal
    destroyOnCloseButton: true
    destroyOnInvisible: true
    frame: ModalFrame{}
}


