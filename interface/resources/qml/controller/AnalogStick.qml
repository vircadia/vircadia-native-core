import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

Item {
    id: root
    property int size: 64
    width: size
    height: size
    property bool invertY: false


    property int halfSize: size / 2
    property var controlIds: [ 0, 0 ]
    property vector2d value: Qt.vector2d(0, 0)

    function update() {
        value = Qt.vector2d(
            controlIds[0] ? Controller.getValue(controlIds[0]) : 0,
            controlIds[1] ? Controller.getValue(controlIds[1]) : 0
        );
        if (root.invertY) {
            value.y = value.y * -1.0
        }
        canvas.requestPaint();
    }

    Timer {
        interval: 50; running: controlIds; repeat: true
        onTriggered: root.update()
    }

    Canvas {
       id: canvas
       anchors.fill: parent
       antialiasing: false

       onPaint: {
           var ctx = canvas.getContext('2d');
           ctx.save();
           ctx.beginPath();
           ctx.clearRect(0, 0, width, height);
           ctx.fill();
           ctx.translate(root.halfSize, root.halfSize)
           ctx.lineWidth = 4
           ctx.strokeStyle = Qt.rgba(Math.max(Math.abs(value.x), Math.abs(value.y)), 0, 0, 1)
           ctx.moveTo(0, 0).lineTo(root.value.x * root.halfSize, root.value.y * root.halfSize)
           ctx.stroke()
           ctx.restore()
       }
    }
}


