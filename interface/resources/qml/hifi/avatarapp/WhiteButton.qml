import QtQuick 2.5
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit

HifiControlsUit.Button {
    HifiConstants {
        id: hifi
    }

    color: hifi.buttons.noneBorderlessGray;
    colorScheme: hifi.colorSchemes.light;
    height: 40
}
