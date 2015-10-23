import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

Rectangle {
    id: root
    property int size: 64
    width: size
    height: size
    color: 'black'
    property int controlId: 0
    property real value: 0.5
    property int scrollWidth: 1
    property real min: 0.0
    property real max: 1.0
    property bool log: false
    property real range: max - min
    property color lineColor: 'yellow'
    property bool bar: false
    property real lastHeight: -1
    property string label: ""

    function update() {
        value = Controller.getValue(controlId);
        if (log) {
            var log = Math.log(10) / Math.log(Math.abs(value));
            var sign = Math.sign(value);
            value = log * sign;
        }
        canvas.requestPaint();
    }

    function drawHeight() {
        if (value < min) {
            return 0;
        }
        if (value > max) {
            return height;
        }
        return ((value - min) / range) * height;
    }

    Timer {
        interval: 50; running: true; repeat: true
        onTriggered: root.update()
    }

    Canvas {
       id: canvas
       anchors.fill: parent
       antialiasing: false

       Text {
           anchors.top: parent.top
           text: root.label
           color: 'white'
       }

       Text {
           anchors.right: parent.right
           anchors.top: parent.top
           text: root.max
           color: 'white'
       }

       Text {
           anchors.right: parent.right
           anchors.bottom: parent.bottom
           text: root.min
           color: 'white'
       }

       function scroll() {
           var ctx = canvas.getContext('2d');
           var image = ctx.getImageData(0, 0, canvas.width, canvas.height);
           ctx.beginPath();
           ctx.clearRect(0, 0, canvas.width, canvas.height);
           ctx.drawImage(image, -root.scrollWidth, 0, canvas.width, canvas.height)
           ctx.restore()
       }

       onPaint: {
           scroll();
           var ctx = canvas.getContext('2d');
           ctx.save();
           var currentHeight = root.drawHeight();
           if (root.lastHeight == -1) {
               root.lastHeight = currentHeight
           }

//           var x = canvas.width - root.drawWidth;
//           var y = canvas.height - drawHeight;
//           ctx.fillStyle = root.color
//           ctx.fillRect(x, y, root.drawWidth, root.bar ? drawHeight : 1)
//           ctx.fill();
//           ctx.restore()


           ctx.beginPath();
           ctx.lineWidth = 1
           ctx.strokeStyle = root.lineColor
           ctx.moveTo(canvas.width - root.scrollWidth, root.lastHeight).lineTo(canvas.width, currentHeight)
           ctx.stroke()
           ctx.restore()
           root.lastHeight = currentHeight
       }
    }
}


