
import QtQuick.Controls 1.3 as Original
import QtQuick.Controls.Styles 1.3

import "../styles"
import "."

Original.SpinBox {
    style: SpinBoxStyle {
        HifiConstants { id: hifi }
        font.family: hifi.fonts.fontFamily
        font.pixelSize: hifi.fonts.pixelSize
    }
    
}
