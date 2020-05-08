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

        var fields = ["alpha", "alphaVar", "centerId", "upId", "downId", "leftId", "rightId", "upLeftId", "upRightId", "downLeftId", "downRightId"];

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

    NumberArrayField {
        id: alphaField
        key: "alpha"
        value: [0, 0, 0]
    }

    StringField {
        id: alphaVarField
        key: "alphaVar"
        value: ""
        optional: true
    }

    StringField {
        id: centerIdField
        key: "centerId"
        value: ""
        optional: true
    }

    StringField {
        id: upIdField
        key: "upId"
        value: ""
        optional: true
    }

    StringField {
        id: downIdField
        key: "downId"
        value: ""
        optional: true
    }

    StringField {
        id: leftIdField
        key: "leftId"
        value: ""
        optional: true
    }

    StringField {
        id: rightIdField
        key: "rightId"
        value: ""
        optional: true
    }

    StringField {
        id: upLeftIdField
        key: "upLeftId"
        value: ""
        optional: true
    }

    StringField {
        id: upRightIdField
        key: "upRightId"
        value: ""
        optional: true
    }

    StringField {
        id: downLeftIdField
        key: "downLeftId"
        value: ""
        optional: true
    }

    StringField {
        id: downRightIdField
        key: "downRightId"
        value: ""
        optional: true
    }
}

