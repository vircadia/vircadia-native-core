import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

ApplicationWindow {
    id: window
    visible: true

    Timer {
        interval: 50; running: true; repeat: true
        onTriggered: {
            Controller.update();
        }
    }


    Loader {
        id: pageLoader
        source: ResourcePath + "TestControllers.qml"
    }
}
