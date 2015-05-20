import QtQuick 2.4
import "../styles"

Item {
    id: root
    HifiConstants { id: hifi }
    property real size: hifi.layout.spacing
    property real multiplier: 1.0
    height: size * multiplier
    width: size * multiplier
}
