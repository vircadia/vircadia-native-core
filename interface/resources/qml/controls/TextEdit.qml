import QtQuick 2.3 as Original
import "../styles"

Original.TextEdit {
    HifiConstants { id: hifi }
    font.family: hifi.fonts.fontFamily
    font.pointSize: hifi.fonts.fontSize
}

