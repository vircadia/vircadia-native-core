import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

ApplicationWindow {
    id: window
    visible: true

    Loader {
        id: pageLoader
        source: "content.qml"
    }
}
