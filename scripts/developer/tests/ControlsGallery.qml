import QtQuick 2.10
import QtQuick.Window 2.10
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "qrc:////qml//styles-uit" as HifiStylesUit
import "qrc:////qml//controls-uit" as HifiControlsUit

//uncomment to use from qmlscratch tool
//import '../../../interface/resources/qml/controls-uit' as HifiControlsUit
//import '../../../interface/resources/qml/styles-uit'

//uncomment to use with HIFI_USE_SOURCE_TREE_RESOURCES=1
//import '../../../resources/qml/controls-uit' as HifiControlsUit
//import '../../../resources/qml/styles-uit'

Item {
    visible: true
    width: 640
    height: 480

    Introspector {
        id: introspector
        properties: ['realFrom', 'realTo', 'realValue', 'realStepSize', 'decimals']
        visible: true
        y: 50
        x: 130
    }

    HifiStylesUit.HifiConstants {
        id: hifi
    }

    TabBar {
        id: bar
        width: parent.width
        TabButton {
            text: "Spinbox"
        }
        TabButton {
            text: "... Other Controls"
        }
    }

    StackLayout {
        id: controlsLayout
        currentIndex: bar.currentIndex
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20

        Item {
            id: spinboxTab
            anchors.fill: parent

            Column {
                spacing: 20

                HifiControlsUit.SpinBox {
                    realValue: 5.0
                    realFrom: 16.0
                    realTo: 20.0
                    decimals: 2
                    realStepSize: 0.01

                    width: 100
                    height: 30

                    colorScheme: hifi.colorSchemes.dark

                    onFocusChanged: {
                        if(focus) {
                            introspector.object = this
                        }
                    }
                }

                HifiControlsUit.SpinBox {
                    realValue: 5.0
                    realFrom: 1.0
                    realTo: 20.0
                    decimals: 2
                    realStepSize: 0.01

                    width: 100
                    height: 30

                    colorScheme: hifi.colorSchemes.light

                    onFocusChanged: {
                        if(focus) {
                            introspector.object = this
                        }
                    }
                }
            }
        }
        Item {
            id: otherTab
        }
    }
}
