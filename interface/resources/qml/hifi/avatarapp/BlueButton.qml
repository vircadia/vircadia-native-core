import QtQuick 2.5
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit

HifiControlsUit.Button {
    HifiConstants {
        id: hifi
    }

    width: Math.max(hifi.dimensions.buttonWidth, implicitTextWidth + 20)
    fontSize: 18
    color: hifi.buttons.blue;
    colorScheme: hifi.colorSchemes.light;
    height: 40
}
