import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

Item {
    id: root
    width: size
    height: size
    property int size: 64
    property int controlId: 0
    property real value: 0
    property color color: 'black'

    Timer {
        interval: 50; running: true; repeat: true
        onTriggered: {
            root.value = Controller.getValue(root.controlId);
            canvas.requestPaint();
        }
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
           if (root.value > 0.0) {
               ctx.fillStyle = root.color
               ctx.fillRect(0, 0, canvas.width, canvas.height);
           }
           ctx.fill();
           ctx.restore()
       }
    }
}


