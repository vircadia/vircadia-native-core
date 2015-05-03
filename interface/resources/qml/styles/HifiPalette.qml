import QtQuick 2.4

Item {
    property string hifiBlue: "#0e7077"
    property alias colors: colorsObj

    Item {
        id: colorsObj
        property string hifiRed: "red"
    }
}
