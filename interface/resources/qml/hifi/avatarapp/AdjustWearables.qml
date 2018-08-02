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

    signal adjustWearablesOpened(var avatarName);
    signal adjustWearablesClosed(bool status, var avatarName);

    property bool modified: false;
    Component.onCompleted: {
        modified = false;
    }

    property var jointNames;
    property string avatarName: ''
    property var wearablesModel;

    function open(avatar) {
        adjustWearablesOpened(avatar.name);

        visible = true;
        avatarName = avatar.name;
        wearablesModel = avatar.wearables;
        refresh(avatar);
    }

    function refresh(avatar) {
        wearablesCombobox.model.clear();
        for(var i = 0; i < avatar.wearables.count; ++i) {
            var wearable = avatar.wearables.get(i).properties;
            for(var j = (wearable.modelURL.length - 1); j >= 0; --j) {
                if(wearable.modelURL[j] === '/') {
                    wearable.text = wearable.modelURL.substring(j + 1) + ' [%jointIndex%]'.replace('%jointIndex%', jointNames[wearable.parentJointIndex]);
                    break;
                }
            }
            wearablesCombobox.model.append(wearable);
        }

        wearablesCombobox.currentIndex = 0;
    }

    function refreshWearable(wearableID, wearableIndex, properties, updateUI) {
        if(wearableIndex === -1) {
            wearableIndex = wearablesCombobox.model.findIndexById(wearableID);
        }

        var wearable = wearablesCombobox.model.get(wearableIndex);

        if(!wearable) {
            return;
        }

        var wearableModelItemProperties = wearablesModel.get(wearableIndex).properties;

        for(var prop in properties) {
            wearable[prop] = properties[prop];
            wearableModelItemProperties[prop] = wearable[prop];

            if(updateUI) {
                if(prop === 'localPosition') {
                    position.set(wearable[prop]);
                } else if(prop === 'localRotationAngles') {
                    rotation.set(wearable[prop]);
                } else if(prop === 'dimensions') {
                    scalespinner.set(wearable[prop].x / wearable.naturalDimensions.x);
                }
            }
        }

        wearablesModel.setProperty(wearableIndex, 'properties', wearableModelItemProperties);
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

    function close(status) {
        visible = false;
        adjustWearablesClosed(status, avatarName);
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
                function findIndexById(id) {

                    for(var i = 0; i < count; ++i) {
                        var wearable = get(i);
                        if(wearable.id === id) {
                            return i;
                        }
                    }

                    return -1;
                }
            }

            comboBox.onCurrentIndexChanged: {
                var currentWearable = getCurrentWearable();

                if(currentWearable) {
                    position.set(currentWearable.localPosition);
                    rotation.set(currentWearable.localRotationAngles);
                    scalespinner.set(currentWearable.dimensions.x / currentWearable.naturalDimensions.x)

                    wearableSelected(currentWearable.id);
                }
            }
        }

        Column {
            width: parent.width
            spacing: 5

            Row {
                spacing: 20

                // TextStyle5
                FiraSansSemiBold {
                    id: positionLabel
                    size: 22;
                    text: "Position"
                }

                // TextStyle7
                FiraSansRegular {
                    size: 18;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 16.9;
                    text: "m"
                    anchors.verticalCenter: positionLabel.verticalCenter
                }
            }

            Vector3 {
                id: position
                backgroundColor: "lightgray"

                function set(localPosition) {
                    notify = false;
                    xvalue = localPosition.x
                    yvalue = localPosition.y
                    zvalue = localPosition.z
                    notify = true;
                }

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

                // TextStyle5
                FiraSansSemiBold {
                    id: rotationLabel
                    size: 22;
                    text: "Rotation"
                }

                // TextStyle7
                FiraSansRegular {
                    size: 18;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 16.9;
                    text: "deg"
                    anchors.verticalCenter: rotationLabel.verticalCenter
                }
            }

            Vector3 {
                id: rotation
                backgroundColor: "lightgray"

                function set(localRotationAngles) {
                    notify = false;
                    xvalue = localRotationAngles.x
                    yvalue = localRotationAngles.y
                    zvalue = localRotationAngles.z
                    notify = true;
                }

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

                decimals: 0
                realFrom: -180
                realTo: 180
                realStepSize: 1
            }
        }

        Column {
            width: parent.width
            spacing: 5

            // TextStyle5
            FiraSansSemiBold {
                size: 22;
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

                    function set(value) {
                        notify = false;
                        realValue = value
                        notify = true;
                    }

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
                    fontSize: 18
                    height: 40
                    anchors.right: parent.right
                    color: hifi.buttons.red;
                    colorScheme: hifi.colorSchemes.light;
                    text: "TAKE IT OFF"
                    onClicked: wearableDeleted(root.avatarName, getCurrentWearable().id);
                    enabled: wearablesCombobox.model.count !== 0
                    anchors.verticalCenter: scalespinner.verticalCenter
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
            root.close(true);
        }

        onNoClicked: function() {
            root.close(false);
        }
    }
}
