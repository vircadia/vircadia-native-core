import QtQuick 2.3 as Original
import "../styles"

Original.TextEdit {
    HifiConstants { id: hifi }
    font.family: hifi.fonts.fontFamily
    font.pixelSize: hifi.fonts.pixelSize
}

