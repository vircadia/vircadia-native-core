import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import com.highfidelity.test 1.0

ApplicationWindow {
    id: window
    visible: true

    AppHook {
        
    }
    
//    NewControllers {
//        id: newControllers
//    }
    
    Rectangle {
        id: page
        width: 320; height: 480
        color: "lightgray"
        Text {
            id: helloText
            text: "Hello world!"
            y: 30
            anchors.horizontalCenter: page.horizontalCenter
            font.pointSize: 24; font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                var newMapping = NewControllers.newMapping();
                console.log("Mapping Object " + newMapping);
                var routeBuilder = newMapping.from("Hello");
                console.log("Route Builder " + routeBuilder);
                routeBuilder.clamp(0, 1).clamp(0, 1).to("Goodbye");
            }
        }
    }

}
