import Hifi 1.0 as Hifi
import QtQuick 2.5
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

Rectangle {
    id: root;
    visible: false;
    width: 480
    height: 706
    color: 'white'

    signal wearableChanged(var id, var properties);

    property bool modified: false;
    Component.onCompleted: {
        modified = false;
    }

    onModifiedChanged: {
        console.debug('modified: ', modified)
    }

    property var onButton2Clicked;
    property var onButton1Clicked;
    property var jointNames;

    property var wearables: ({})
    function open(avatar) {
        console.debug('AdjustWearables.qml: open: ', JSON.stringify(avatar, null, '\t'));

        visible = true;
        wearablesCombobox.model.clear();
        wearables = {};

        console.debug('AdjustWearables.qml: avatar.wearables.count: ', avatar.wearables.count);
        for(var i = 0; i < avatar.wearables.count; ++i) {
            var wearable = avatar.wearables.get(i).properties;
            console.debug('wearable: ', JSON.stringify(wearable, null, '\t'))

            for(var j = (wearable.modelURL.length - 1); j >= 0; --j) {
                if(wearable.modelURL[j] === '/') {
                    wearable.text = wearable.modelURL.substring(j + 1) + ' [%jointIndex%]'.replace('%jointIndex%', jointNames[wearable.parentJointIndex]);
                    wearables[wearable.id] = {
                        position: wearable.localPosition,
                        rotation: wearable.localRotation,
                        dimensions: wearable.dimensions
                    };

                    console.debug('wearable.text = ', wearable.text);
                    break;
                }
            }
            wearablesCombobox.model.append(wearable);
        }

        wearablesCombobox.currentIndex = 0;
    }

    function close() {
        visible = false;
    }

    HifiConstants { id: hifi }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 15
        anchors.horizontalCenter: parent.horizontalCenter

        spacing: 20
        width: parent.width - 30 * 2

        HifiControlsUit.ComboBox {
            id: wearablesCombobox
            anchors.left: parent.left
            anchors.right: parent.right
            comboBox.textRole: "text"

            model: ListModel {
            }

            comboBox.onCurrentIndexChanged: {
                console.debug('wearable index changed: ', currentIndex);
                var currentWearable = wearablesCombobox.model.get(currentIndex)

                if(currentWearable) {
                    position.notify = false;
                    position.xvalue = currentWearable.localPosition.x
                    position.yvalue = currentWearable.localPosition.y
                    position.zvalue = currentWearable.localPosition.z
                    position.notify = true;

                    rotation.notify = false;
                    rotation.xvalue = currentWearable.localRotationAngles.x
                    rotation.yvalue = currentWearable.localRotationAngles.y
                    rotation.zvalue = currentWearable.localRotationAngles.z
                    rotation.notify = true;
                }
            }
        }

        Column {
            width: parent.width
            spacing: 5

            Row {
                spacing: 20

                TextStyle5 {
                    text: "Position"
                }

                TextStyle7 {
                    text: "m"
                }
            }

            Vector3 {
                id: position
                backgroundColor: "lightgray"

                function notifyPositionChanged() {
                    modified = true;
                    var properties = {};
                    properties.localPosition = { 'x' : xvalue, 'y' : yvalue, 'z' : zvalue }

                    var currentWearable = wearablesCombobox.model.get(wearablesCombobox.currentIndex)
                    wearableChanged(currentWearable.id, properties);
                }

                property bool notify: false;

                onXvalueChanged: if(notify) notifyPositionChanged();
                onYvalueChanged: if(notify) notifyPositionChanged();
                onZvalueChanged: if(notify) notifyPositionChanged();

                decimals: 4
                realFrom: -100
                realTo: 100
                realStepSize: 0.0001
            }
        }

        Column {
            width: parent.width
            spacing: 5

            Row {
                spacing: 20

                TextStyle5 {
                    text: "Rotation"
                }

                TextStyle7 {
                    text: "deg"
                }
            }

            Vector3 {
                id: rotation
                backgroundColor: "lightgray"

                function notifyRotationChanged() {
                    modified = true;
                    var properties = {};
                    properties.localRotationAngles = { 'x' : xvalue, 'y' : yvalue, 'z' : zvalue }

                    var currentWearable = wearablesCombobox.model.get(wearablesCombobox.currentIndex)
                    wearableChanged(currentWearable.id, properties);
                }

                property bool notify: false;

                onXvalueChanged: if(notify) notifyRotationChanged();
                onYvalueChanged: if(notify) notifyRotationChanged();
                onZvalueChanged: if(notify) notifyRotationChanged();

                decimals: 4
                realFrom: -100
                realTo: 100
                realStepSize: 0.0001
            }
        }

        Column {
            width: parent.width
            spacing: 5

            TextStyle5 {
                text: "Scale"
            }

            Item {
                width: parent.width
                height: childrenRect.height

                HifiControlsUit.SpinBox {
                    id: scalespinner
                    value: 0
                    backgroundColor: "lightgray"
                    width: position.spinboxWidth
                    colorScheme: hifi.colorSchemes.light
                    onValueChanged: modified = true;
                }

                HifiControlsUit.Button {
                    anchors.right: parent.right
                    color: hifi.buttons.red;
                    colorScheme: hifi.colorSchemes.dark;
                    text: "TAKE IT OFF"
                }
            }

        }
    }

    DialogButtons {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.right: parent.right
        anchors.rightMargin: 30

        yesText: "SAVE"
        noText: "CANCEL"

        onYesClicked: function() {
            if(onButton2Clicked) {
                onButton2Clicked();
            } else {
                root.close();
            }
        }

        onNoClicked: function() {
            if(onButton1Clicked) {
                onButton1Clicked();
            } else {
                root.close();
            }
        }
    }
}
