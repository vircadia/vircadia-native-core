import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 1.6
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.0
import "../fields"

Column {
    id: column
    x: 0
    y: 0
    height: 50
    width: 200
    spacing: 6
    property var modelIndex

    signal fieldChanged(string key, var newValue)

    // called by each field when its value is changed.
    function fieldChangedMethod(key, newValue) {
        var ROLE_DATA = 0x0103;
        var dataValue = theModel.data(modelIndex, ROLE_DATA);
        dataValue[key] = newValue;

        // copy the new value into the model.
        theModel.setData(modelIndex, dataValue, ROLE_DATA);
    }

    // called when a new model is loaded
    function setIndex(index) {
        modelIndex = index;

        var ROLE_DATA = 0x0103;
        var dataValue = theModel.data(modelIndex, ROLE_DATA);

        var fields = ["alpha", "enabled", "interpDuration", "baseJointName", "midJointName", "tipJointName",
                      "basePositionVar", "baseRotationVar", "midPositionVar", "midRotationVar", "tipPositionVar", "tipRotationVar",
                      "alphaVar", "enabledVar"];

        // copy data from theModel into each field.
        var l = fields.length;
        for (var i = 0; i < l; i++) {
            var val = dataValue[fields[i]];
            if (val) {
                column.children[i].value = val;
            }
        }
    }

    Component.onCompleted: {
        // this signal is fired from each data field when values are changed.
        column.fieldChanged.connect(fieldChangedMethod)
    }

    NumberField {
        id: alphaField
        key: "alpha"
        value: 1.0
    }

    BooleanField {
        id: enabledField
        key: "enabled"
        value: false
    }

    NumberField {
        id: interpDurationField
        key: "interpDuration"
        value: 15.0
    }

    StringField {
        id: baseJointNameField
        key: "baseJointName"
        value: ""
    }

    StringField {
        id: midJointNameField
        key: "midJointName"
        value: ""
    }

    StringField {
        id: tipJointNameField
        key: "tipJointName"
        value: ""
    }

    StringField {
        id: basePositionVarField
        key: "basePositionVar"
        value: ""
    }

    StringField {
        id: baseRotationVarField
        key: "baseRotationVar"
        value: ""
    }

    StringField {
        id: midPositionVarField
        key: "midPositionVar"
        value: ""
    }

    StringField {
        id: midRotationVarField
        key: "midRotationVar"
        value: ""
    }

    StringField {
        id: tipPositionVarField
        key: "tipPositionVar"
        value: ""
    }

    StringField {
        id: tipRotationVarField
        key: "tipRotationVar"
        value: ""
    }

    StringField {
        id: alphaVarField
        key: "alphaVar"
        value: ""
    }

    StringField {
        id: enabledVarField
        key: "enabledVar"
        value: ""
    }
}

