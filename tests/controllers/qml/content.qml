import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

Rectangle {
    id: root
    implicitHeight: column1.height + 24
    implicitWidth: column1.width + 24
    color: "lightgray"

    property real itemSize: 128

    Component {
        id: graphTemplate
        Item {
            implicitHeight: canvas.height + 2 + text.height
            implicitWidth: canvas.width
            property string text: loadText

            Canvas {
               id: canvas
               width: root.itemSize; height: root.itemSize;
               antialiasing: false
               property int controlId: control
               property real value: 0.0
               property int drawWidth: 1

               Timer {
                   interval: 50; running: true; repeat: true
                   onTriggered: {
                       parent.value = NewControllers.getValue(canvas.controlId)
                       parent.requestPaint();
                   }
               }

               onPaint: {
                   var ctx = canvas.getContext('2d');
                   ctx.save();

                   var image = ctx.getImageData(0, 0, canvas.width, canvas.height);
                   ctx.clearRect(0, 0, canvas.width, canvas.height);
                   ctx.drawImage(image, -drawWidth, 0, canvas.width, canvas.height)
                   ctx.fillStyle = 'green'
                   // draw a filles rectangle
                   var height = canvas.height * canvas.value
                   ctx.fillRect(canvas.width - drawWidth, canvas.height - height,
                                drawWidth, height)
                   ctx.restore()
               }
            }

            Text {
                id: text
                text: parent.text
                anchors.topMargin: 2
                anchors.horizontalCenter: canvas.horizontalCenter
                anchors.top: canvas.bottom
                font.pointSize: 12;
            }

        }
    }

    Column {
        id: column1
        x: 12; y: 12
        spacing: 24
        Row {
            spacing: 16
            Loader {
                sourceComponent: graphTemplate;
                property string loadText: "Key Left"
                property int control: ControllerIds.Hardware.Keyboard2.Left
            }
            Loader {
                sourceComponent: graphTemplate;
                property string loadText: "DPad Up"
                property int control: ControllerIds.Hardware.X360Controller1.DPadUp
            }
            /*
            Loader {
                sourceComponent: graphTemplate;
                property string loadText: "Yaw Left"
                property int control: ControllerIds.Actions.YAW_LEFT
            }
            Loader {
                sourceComponent: graphTemplate;
                property string loadText: "Yaw Left"
                property int control: ControllerIds.Actions.YAW_LEFT
            }
*/

//            Loader { sourceComponent: graphTemplate; }
//            Loader { sourceComponent: graphTemplate; }
//            Loader { sourceComponent: graphTemplate; }
        }
        /*
        Row {
            spacing: 16
            Loader { sourceComponent: graphTemplate; }
            Loader { sourceComponent: graphTemplate; }
            Loader { sourceComponent: graphTemplate; }
            Loader { sourceComponent: graphTemplate; }
        }
        Row {
            spacing: 16
            Loader { sourceComponent: graphTemplate; }
            Loader { sourceComponent: graphTemplate; }
            Loader { sourceComponent: graphTemplate; }
            Loader { sourceComponent: graphTemplate; }
        }
        */


        Button {
            text: "Go!"
            onClicked: {
                //

//                var newMapping = NewControllers.newMapping();
//                console.log("Mapping Object " + newMapping);
//                var routeBuilder = newMapping.from("Hello");
//                console.log("Route Builder " + routeBuilder);
//                routeBuilder.clamp(0, 1).clamp(0, 1).to("Goodbye");
            }
        }

        Timer {
            interval: 50; running: true; repeat: true
            onTriggered: {
                NewControllers.update();
            }
        }

    }
}
