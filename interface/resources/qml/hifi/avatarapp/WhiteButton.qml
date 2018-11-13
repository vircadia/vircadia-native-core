import QtQuick 2.5
import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit

HifiControlsUit.Button {
    HifiConstants {
        id: hifi
    }

    width: Math.max(hifi.dimensions.buttonWidth, implicitTextWidth + 20)
    fontSize: 18
    color: hifi.buttons.noneBorderlessGray;
    colorScheme: hifi.colorSchemes.light;
    height: 40
}
