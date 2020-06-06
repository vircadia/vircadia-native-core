import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Layouts 1.3
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit

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
    signal addWearable(string avatarName, string url);

    property bool modified: false;
    Component.onCompleted: {
        modified = false;
    }

    property var jointNames: []
    onJointNamesChanged: {
        jointsModel.clear();
        for (var i = 0; i < jointNames.length; ++i) {
            var jointName = jointNames[i];
            if (jointName !== 'LeftHand' && jointName !== 'RightHand') {
                jointsModel.append({'text' : jointName, 'jointIndex' : i});
            }
        }
    }

    property string avatarName: ''
    property var wearablesModel;

    function open(avatar) {
        adjustWearablesOpened(avatar.name);

        modified = false;
        visible = true;
        avatarName = avatar.name;
        wearablesModel = avatar.wearables;
        refresh(avatar);
    }

    function extractTitleFromUrl(url) {
        for (var j = (url.length - 1); j >= 0; --j) {
            if (url[j] === '/') {
                return url.substring(j + 1);
            }
        }
        return url;
    }

    function refresh(avatar) {
        wearablesCombobox.model.clear();
        wearablesCombobox.currentIndex = -1;

        for (var i = 0; i < avatar.wearables.count; ++i) {
            var wearable = avatar.wearables.get(i).properties;
            if (wearable.modelURL) {
                wearable.text = extractTitleFromUrl(wearable.modelURL);
            } else if (wearable.materialURL) {
                var materialUrlOrJson = '';
                if (!wearable.materialURL.startsWith('materialData')) {
                    materialUrlOrJson = extractTitleFromUrl(wearable.materialURL);
                } else if (wearable.materialData) {
                    materialUrlOrJson = JSON.stringify(JSON.parse(wearable.materialData))
                }
                if(materialUrlOrJson) {
                    wearable.text = 'Material: ' + materialUrlOrJson;
                }
            } else if (wearable.sourceUrl) {
                wearable.text = extractTitleFromUrl(wearable.sourceUrl);
            } else if (wearable.name) {
                wearable.text = wearable.name;
            }
            wearablesCombobox.model.append(wearable);
        }

        if (wearablesCombobox.model.count !== 0) {
            wearablesCombobox.currentIndex = 0;
        }
    }

    function refreshWearable(wearableID, wearableIndex, properties, updateUI) {
        if (wearableIndex === -1) {
            wearableIndex = wearablesCombobox.model.findIndexById(wearableID);
        }

        var wearable = wearablesCombobox.model.get(wearableIndex);

        if (!wearable) {
            return;
        }

        var wearableModelItemProperties = wearablesModel.get(wearableIndex).properties;

        for (var prop in properties) {
            wearable[prop] = properties[prop];
            wearableModelItemProperties[prop] = wearable[prop];

            if (updateUI) {
                if (prop === 'localPosition') {
                    positionVector.set(wearable[prop]);
                } else if (prop === 'localRotationAngles') {
                    rotationVector.set(wearable[prop]);
                } else if (prop === 'dimensions') {
                    scalespinner.set(wearable[prop].x / wearable.naturalDimensions.x);
                }
                modified = true;
            }
        }

        wearablesModel.setProperty(wearableIndex, 'properties', wearableModelItemProperties);
    }

    function entityHasAvatarJoints(entityID) {
        var hasAvatarJoint = false;

        var props = Entities.getEntityProperties(entityID);
        var avatarJointsCount = MyAvatar.getJointNames().length;
        if (props && avatarJointsCount >= 0 ) {
            var entityJointNames = Entities.getJointNames(entityID);
            for (var index = 0; index < entityJointNames.length; index++) {
                var avatarJointIndex = MyAvatar.getJointIndex(entityJointNames[index]);
                if (avatarJointIndex >= 0) {
                    hasAvatarJoint = true;
                    break;
                }
            }
        }

        return hasAvatarJoint;
    }

    function getCurrentWearable() {
        return wearablesCombobox.currentIndex !== -1 ? wearablesCombobox.model.get(wearablesCombobox.currentIndex) : null;
    }

    function selectWearableByID(entityID) {
        for (var i = 0; i < wearablesCombobox.model.count; ++i) {
            var wearable = wearablesCombobox.model.get(i);
            if (wearable.id === entityID) {
                wearablesCombobox.currentIndex = i;
                softWearableTimer.restart();
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

    Timer {
        id: softWearableTimer
        interval: 1000
        running: false
        repeat: true
        onTriggered: {
            var currentWearable = getCurrentWearable();
            var hasSoft = currentWearable && currentWearable.relayParentJoints !== undefined;
            var soft = hasSoft ? currentWearable.relayParentJoints : false;
            var softEnabled = hasSoft ? entityHasAvatarJoints(currentWearable.id) : false;
            isSoft.set(soft);
            isSoft.enabled = softEnabled;
        }
    }

    Column {
        anchors.top: parent.top
        anchors.topMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter

        spacing: 20
        width: parent.width - 22 * 2

        Column {
            width: parent.width

            Rectangle {
                color: hifi.colors.orangeHighlight
                anchors.left: parent.left
                anchors.right: parent.right
                height: 70
                visible: HMD.active

                RalewayRegular {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    size: 20;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 23;
                    text: "Tip: You can use hand controllers to grab and adjust your wearables"
                    wrapMode: Text.WordWrap
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                height: 12 // spacing
                visible: HMD.active
            }

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                RalewayBold {
                    size: 15;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 18;
                    text: "Wearable"
                    Layout.alignment: Qt.AlignVCenter
                }

                spacing: 10

                RalewayBold {
                    size: 15;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 18;
                    text: "<a href='#'>Get more</a>"
                    linkColor: hifi.colors.blueHighlight
                    Layout.alignment: Qt.AlignVCenter
                    onLinkActivated: {
                        popup.showGetWearables(null, function(link) {
                            emitSendToScript({'method' : 'navigate', 'url' : link})
                        });
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    RalewayBold {
                        size: 15;
                        lineHeightMode: Text.FixedHeight
                        lineHeight: 18;
                        text: "<a href='#'>Add custom</a>"
                        linkColor: hifi.colors.blueHighlight
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        onLinkActivated: {
                            popup.showSpecifyWearableUrl(function(url) {
                                addWearable(root.avatarName, url);
                                modified = true;
                            });
                        }
                    }
                }
            }

            HifiControlsUit.ComboBox {
                id: wearablesCombobox
                anchors.left: parent.left
                anchors.right: parent.right
                enabled: getCurrentWearable() !== null
                comboBox.textRole: "text"

                model: ListModel {
                    function findIndexById(id) {

                        for (var i = 0; i < count; ++i) {
                            var wearable = get(i);
                            if (wearable.id === id) {
                                return i;
                            }
                        }

                        return -1;
                    }
                }

                comboBox.onCurrentIndexChanged: {
                    var currentWearable = getCurrentWearable();
                    var position = currentWearable ? currentWearable.localPosition : { x : 0, y : 0, z : 0 };
                    var rotation = currentWearable ? currentWearable.localRotationAngles : { x : 0, y : 0, z : 0 };
                    var scale = currentWearable ? currentWearable.dimensions.x / currentWearable.naturalDimensions.x : 1.0;
                    var joint = currentWearable ? currentWearable.parentJointIndex : -1;
                    softWearableTimer.restart();

                    positionVector.set(position);
                    rotationVector.set(rotation);
                    scalespinner.set(scale);
                    jointsCombobox.set(joint);


                    if (currentWearable) {
                        wearableSelected(currentWearable.id);
                    }
                }
            }
        }

        Column {
            width: parent.width

            RalewayBold {
                size: 15;
                lineHeightMode: Text.FixedHeight
                lineHeight: 18;
                text: "Joint"
            }

            HifiControlsUit.ComboBox {
                id: jointsCombobox
                anchors.left: parent.left
                anchors.right: parent.right
                enabled: getCurrentWearable() !== null &&  !isSoft.checked
                comboBox.displayText: isSoft.checked ? 'Hips' : comboBox.currentText
                comboBox.textRole: "text"

                model: ListModel {
                    id: jointsModel
                }
                property bool notify: false

                function set(jointIndex) {
                    notify = false;
                    for (var i = 0; i < jointsModel.count; ++i) {
                        if (jointsModel.get(i).jointIndex === jointIndex) {
                            currentIndex = i;
                            break;
                        }
                    }
                    notify = true;
                }

                function notifyJointChanged() {
                    modified = true;
                    var jointIndex = jointsModel.get(jointsCombobox.currentIndex).jointIndex;

                    var properties = {
                        parentJointIndex: jointIndex,
                        localPosition: {
                            x: positionVector.xvalue,
                            y: positionVector.yvalue,
                            z: positionVector.zvalue
                        },
                        localRotationAngles: {
                            x: rotationVector.xvalue,
                            y: rotationVector.yvalue,
                            z: rotationVector.zvalue,
                        }
                    };

                    wearableUpdated(getCurrentWearable().id, wearablesCombobox.currentIndex, properties);
                }

                onCurrentIndexChanged: {
                    if (notify) notifyJointChanged();
                }
            }
        }

        Column {
            width: parent.width

            Row {
                spacing: 20

                // TextStyle5
                RalewayBold {
                    id: positionLabel
                    size: 15;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 18;
                    text: "Position"
                }

                // TextStyle7
                RalewayBold {
                    size: 15;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 18;
                    text: "m"
                    anchors.verticalCenter: positionLabel.verticalCenter
                }
            }

            Vector3 {
                id: positionVector
                enabled: getCurrentWearable() !== null

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

                onXvalueChanged: if (notify) notifyPositionChanged();
                onYvalueChanged: if (notify) notifyPositionChanged();
                onZvalueChanged: if (notify) notifyPositionChanged();

                decimals: 2
                realFrom: -10
                realTo: 10
                realStepSize: 0.01
            }
        }

        Column {
            width: parent.width

            Row {
                spacing: 20

                // TextStyle5
                RalewayBold {
                    id: rotationLabel
                    size: 15;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 18;
                    text: "Rotation"
                }

                // TextStyle7
                RalewayBold {
                    size: 15;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 18;
                    text: "deg"
                    anchors.verticalCenter: rotationLabel.verticalCenter
                }
            }

            Vector3 {
                id: rotationVector
                enabled: getCurrentWearable() !== null

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

                onXvalueChanged: if (notify) notifyRotationChanged();
                onYvalueChanged: if (notify) notifyRotationChanged();
                onZvalueChanged: if (notify) notifyRotationChanged();

                decimals: 0
                realFrom: -180
                realTo: 180
                realStepSize: 1
            }
        }

        Item {
            width: parent.width
            height: childrenRect.height

            HifiControlsUit.CheckBox {
                id: isSoft
                enabled: getCurrentWearable() !== null
                text: "Is soft"
                labelFontSize: 15
                labelFontWeight: Font.Bold
                color:  Qt.black
                y: scalespinner.y

                function set(value) {
                    notify = false;
                    checked = value;
                    notify = true;
                }

                function notifyIsSoftChanged() {
                    modified = true;
                    var properties = {
                        relayParentJoints: checked
                    };

                    wearableUpdated(getCurrentWearable().id, wearablesCombobox.currentIndex, properties);
                }

                property bool notify: false;

                onCheckedChanged: if (notify) notifyIsSoftChanged();
            }

            Column {
                id: scalesColumn
                anchors.right: parent.right

                // TextStyle5
                RalewayBold {
                    id: scaleLabel
                    size: 15;
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 18;
                    text: "Scale"
                }

                HifiControlsUit.SpinBox {
                    id: scalespinner
                    enabled: getCurrentWearable() !== null
                    decimals: 2
                    realStepSize: 0.1
                    realFrom: 0.1
                    realTo: 3.0
                    realValue: 1.0
                    backgroundColor: activeFocus ? "white" : "lightgray"
                    width: positionVector.spinboxWidth
                    colorScheme: hifi.colorSchemes.light

                    property bool notify: false;
                    onRealValueChanged: if (notify) notifyScaleChanged();

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
            }
        }

        Column {
            width: parent.width

            HifiControlsUit.Button {
                fontSize: 18
                height: 40
                width: scalespinner.width
                anchors.right: parent.right
                color: hifi.buttons.red;
                colorScheme: hifi.colorSchemes.light;
                text: "TAKE IT OFF"
                onClicked: {
                    modified = true;
                    wearableDeleted(root.avatarName, getCurrentWearable().id);
                }
                enabled: wearablesCombobox.model.count !== 0
            }
        }
    }

    DialogButtons {
        yesButton.enabled: modified

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 57
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
