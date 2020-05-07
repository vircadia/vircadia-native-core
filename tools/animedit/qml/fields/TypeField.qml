import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 1.6
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.0

Row {
    id: row

    function getTypes() {
        return [
            "clip",
            "blendDirectional",
            "blendLinear",
            "overlay",
            "stateMachine",
            "randomSwitchStateMachine",
            "manipulator",
            "inverseKinematics",
            "defaultPose",
            "twoBoneIK",
            "splineIK",
            "poleVectorConstraint"
        ];
    }

    function indexFromString(str) {
        var index = getTypes().indexOf(str);
        return (index === -1) ? 0 : index;
    }

    x: 0
    y: 0
    height: 20
    anchors.left: parent.left
    anchors.leftMargin: 0
    anchors.right: parent.right
    anchors.rightMargin: 0

    property string theValue: "clip"
    property int theIndex: indexFromString(theValue)

    function setValue(newValue) {
        var ROLE_TYPE = 0x0102;
        theModel.setData(leftHandPane.currentIndex, newValue, ROLE_TYPE);
        rightHandPane.reset();
    }

    Text {
        id: element
        y: 5
        width: 100
        text: qsTr("Type:")
        font.pixelSize: 12
    }

    ComboBox {
        id: comboBox
        x: 100
        width: 200
        model: getTypes()
        currentIndex: theIndex
        onActivated: {
            setValue(currentText);
        }
    }
}
