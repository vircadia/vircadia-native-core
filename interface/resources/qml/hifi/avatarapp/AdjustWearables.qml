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

    signal wearableUpdated(var id, int index, var properties);
    signal wearableSelected(var id);
    signal wearableDeleted(string avatarName, var id);

    signal adjustWearablesOpened();
    signal adjustWearablesClosed();

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
    property string avatarName: ''
    property var wearablesModel;

    function backupWearables(avatar) {
        for(var i = 0; i < avatar.wearables.count; ++i) {
            var wearable = avatar.wearables.get(i).properties;
            wearables[wearable.id] = JSON.stringify(wearable)
        }
    }

    function open(avatar) {
        adjustWearablesOpened();
        console.debug('AdjustWearables.qml: open: ', JSON.stringify(avatar, null, '\t'));

        visible = true;
        avatarName = avatar.name;
        wearablesModel = avatar.wearables;
        wearables = {};

        refresh(avatar);
        backupWearables(avatar);
    }

    function refresh(avatar) {
        wearablesCombobox.model.clear();
        console.debug('AdjustWearables.qml: open: avatar.wearables.count: ', avatar.wearables.count);
        for(var i = 0; i < avatar.wearables.count; ++i) {
            var wearable = avatar.wearables.get(i).properties;
            console.debug('wearable: ', JSON.stringify(wearable, null, '\t'))

            for(var j = (wearable.modelURL.length - 1); j >= 0; --j) {
                if(wearable.modelURL[j] === '/') {
                    wearable.text = wearable.modelURL.substring(j + 1) + ' [%jointIndex%]'.replace('%jointIndex%', jointNames[wearable.parentJointIndex]);
                    console.debug('wearable.text = ', wearable.text);
                    break;
                }
            }
            wearablesCombobox.model.append(wearable);
        }

        wearablesCombobox.currentIndex = 0;
    }

    function refreshWearable(wearableID, wearableIndex, properties) {
        var wearable = wearablesCombobox.model.get(wearableIndex);
        for(var prop in properties) {
            wearable[prop] = properties[prop];

            // 2do: consider removing 'properties' and manipulating localPosition/localRotation directly
            var wearablesModelProps = wearablesModel.get(wearableIndex).properties;
            wearablesModelProps[prop] = properties[prop];
            wearablesModel.setProperty(wearableIndex, 'properties', wearablesModelProps);

            console.debug('updated wearable', prop,
                          'old = ', JSON.stringify(wearable[prop], 0, 4),
                          'new = ', JSON.stringify(properties[prop], 0, 4),
                          'model = ', JSON.stringify(wearablesCombobox.model.get(wearableIndex)[prop]),
                          'wearablesModel = ', JSON.stringify(wearablesModel.get(wearableIndex).properties[prop], 0, 4)
                          );
        }

        console.debug('wearablesModel.get(wearableIndex).properties: ', JSON.stringify(wearablesModel.get(wearableIndex).properties, 0, 4))
    }

    function getCurrentWearable() {
        return wearablesCombobox.model.get(wearablesCombobox.currentIndex)
    }

    function selectWearableByID(entityID) {
        for(var i = 0; i < wearablesCombobox.model.count; ++i) {
            var wearable = wearablesCombobox.model.get(i);
            if(wearable.id === entityID) {
                wearablesCombobox.currentIndex = i;
                break;
            }
        }
    }

    function close() {
        visible = false;
        adjustWearablesClosed();
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

                var currentWearable = getCurrentWearable();

                if(currentWearable) {
                    position.notify = false;
                    position.xvalue = currentWearable.localPosition.x
                    position.yvalue = currentWearable.localPosition.y
                    position.zvalue = currentWearable.localPosition.z
                    console.debug('currentWearable.localPosition = ', JSON.stringify(currentWearable.localPosition, 0, 4))
                    position.notify = true;

                    rotation.notify = false;
                    rotation.xvalue = currentWearable.localRotationAngles.x
                    rotation.yvalue = currentWearable.localRotationAngles.y
                    rotation.zvalue = currentWearable.localRotationAngles.z
                    console.debug('currentWearable.localRotationAngles = ', JSON.stringify(currentWearable.localRotationAngles, 0, 4))
                    rotation.notify = true;

                    scalespinner.notify = false;
                    scalespinner.realValue = currentWearable.dimensions.x / currentWearable.naturalDimensions.x
                    console.debug('currentWearable.scale = ', scalespinner.realValue)
                    scalespinner.notify = true;

                    wearableSelected(currentWearable.id);
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
                    var properties = {
                        localPosition: { 'x' : xvalue, 'y' : yvalue, 'z' : zvalue }
                    };

                    wearableUpdated(getCurrentWearable().id, wearablesCombobox.currentIndex, properties);
                }

                property bool notify: false;

                onXvalueChanged: if(notify) notifyPositionChanged();
                onYvalueChanged: if(notify) notifyPositionChanged();
                onZvalueChanged: if(notify) notifyPositionChanged();

                decimals: 2
                realFrom: -10
                realTo: 10
                realStepSize: 0.01
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
                    var properties = {
                        localRotationAngles: { 'x' : xvalue, 'y' : yvalue, 'z' : zvalue }
                    };

                    wearableUpdated(getCurrentWearable().id, wearablesCombobox.currentIndex, properties);
                }

                property bool notify: false;

                onXvalueChanged: if(notify) notifyRotationChanged();
                onYvalueChanged: if(notify) notifyRotationChanged();
                onZvalueChanged: if(notify) notifyRotationChanged();

                decimals: 2
                realFrom: -10
                realTo: 10
                realStepSize: 0.01
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
                    decimals: 2
                    realStepSize: 0.1
                    realFrom: 0.1
                    realTo: 3.0
                    realValue: 1.0
                    backgroundColor: "lightgray"
                    width: position.spinboxWidth
                    colorScheme: hifi.colorSchemes.light

                    property bool notify: false;
                    onValueChanged: if(notify) notifyScaleChanged();

                    function notifyScaleChanged() {
                        modified = true;
                        var currentWearable = getCurrentWearable();
                        var naturalDimensions = currentWearable.naturalDimensions;

                        var properties = {
                            dimensions: {
                                'x' : realValue * naturalDimensions.x,
                                'y' : realValue * naturalDimensions.y,
                                'z' : realValue * naturalDimensions.z
                            }
                        };

                        wearableUpdated(currentWearable.id, wearablesCombobox.currentIndex, properties);
                    }
                }

                HifiControlsUit.Button {
                    anchors.right: parent.right
                    color: hifi.buttons.red;
                    colorScheme: hifi.colorSchemes.dark;
                    text: "TAKE IT OFF"
                    onClicked: wearableDeleted(root.avatarName, getCurrentWearable().id);
                    enabled: wearablesCombobox.model.count !== 0
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
