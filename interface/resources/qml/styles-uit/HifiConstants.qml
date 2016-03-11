//
//  HiFiConstants.qml
//
//  Created by Bradley Austin Davis on 28 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Window 2.2

Item {
    readonly property alias colors: colors
    readonly property alias colorSchemes: colorSchemes
    readonly property alias dimensions: dimensions
    readonly property alias fontSizes: fontSizes
    readonly property alias glyphs: glyphs
    readonly property alias icons: icons
    readonly property alias buttons: buttons
    readonly property alias effects: effects

    function glyphForIcon(icon) {
        // Translates icon enum to glyph char.
        var glyph;
        switch (icon) {
            case hifi.icons.information:
                glyph = hifi.glyphs.info;
                break;
            case hifi.icons.question:
                glyph = hifi.glyphs.question;
                break;
            case hifi.icons.warning:
                glyph = hifi.glyphs.alert;
                break;
            case hifi.icons.critical:
                glyph = hifi.glyphs.critical;
                break;
            case hifi.icons.placemark:
                glyph = hifi.glyphs.placemark;
                break;
            default:
                glyph = hifi.glyphs.noIcon;
        }
        return glyph;
    }

    Item {
        id: colors

        // Base colors
        readonly property color baseGray: "#404040"
        readonly property color darkGray: "#121212"
        readonly property color baseGrayShadow: "#252525"
        readonly property color baseGrayHighlight: "#575757"
        readonly property color lightGray: "#6a6a6a"
        readonly property color lightGrayText: "#afafaf"
        readonly property color faintGray: "#e3e3e3"
        readonly property color primaryHighlight: "#00b4ef"
        readonly property color blueAccent: "#1080b8"
        readonly property color redHighlight: "#e2334d"
        readonly property color redAccent: "#b70a37"
        readonly property color greenHighlight: "#1ac567"
        readonly property color greenShadow: "#2c8e72"
        // Semitransparent
        readonly property color darkGray30: "#4d121212"
        readonly property color darkGray0: "#00121212"
        readonly property color baseGrayShadow60: "#99252525"
        readonly property color baseGrayShadow50: "#80252525"
        readonly property color baseGrayShadow25: "#40252525"
        readonly property color baseGrayHighlight40: "#66575757"
        readonly property color baseGrayHighlight15: "#26575757"
        readonly property color lightGray50: "#806a6a6a"
        readonly property color lightGrayText80: "#ccafafaf"
        readonly property color faintGray80: "#cce3e3e3"
        readonly property color faintGray50: "#80e3e3e3"

        // Other colors
        readonly property color white: "#ffffff"
        readonly property color gray: "#808080"
        readonly property color black: "#000000"
        // Semitransparent
        readonly property color white50: "#80ffffff"
        readonly property color white30: "#4dffffff"
        readonly property color white25: "#40ffffff"
        readonly property color transparent: "#00ffffff"

        // Control specific colors
        readonly property color tableRowLightOdd: white50
        readonly property color tableRowLightEven: "#1a575757"
        readonly property color tableRowDarkOdd: "#80393939"
        readonly property color tableRowDarkEven: "#a6181818"
        readonly property color tableScrollHandle: "#707070"
        readonly property color tableScrollBackground: "#323232"
        readonly property color checkboxLightStart: "#ffffff"
        readonly property color checkboxLightFinish: "#afafaf"
        readonly property color checkboxDarkStart: "#7d7d7d"
        readonly property color checkboxDarkFinish: "#6b6a6b"
        readonly property color checkboxChecked: primaryHighlight
        readonly property color checkboxCheckedBorder: "#36cdff"
        readonly property color sliderGutterLight: "#d4d4d4"
        readonly property color sliderGutterDark: "#252525"
        readonly property color sliderBorderLight: "#afafaf"
        readonly property color sliderBorderDark: "#7d7d7d"
        readonly property color sliderLightStart: "#ffffff"
        readonly property color sliderLightFinish: "#afafaf"
        readonly property color sliderDarkStart: "#7d7d7d"
        readonly property color sliderDarkFinish: "#6b6a6b"
        readonly property color dropDownPressedLight: "#d4d4d4"
        readonly property color dropDownPressedDark: "#afafaf"
        readonly property color dropDownLightStart: "#ffffff"
        readonly property color dropDownLightFinish: "#afafaf"
        readonly property color dropDownDarkStart: "#7d7d7d"
        readonly property color dropDownDarkFinish: "#6b6a6b"
    }

    Item {
        id: colorSchemes
        readonly property int light: 0
        readonly property int dark: 1
    }

    Item {
        id: dimensions
        readonly property bool largeScreen: Screen.width >= 1920 && Screen.height >= 1080
        readonly property real borderRadius: largeScreen ? 7.5 : 5.0
        readonly property real borderWidth: largeScreen ? 2 : 1
        readonly property vector2d contentMargin: Qt.vector2d(12, 24)
        readonly property vector2d contentSpacing: Qt.vector2d(8, 12)
        readonly property real labelPadding: 40
        readonly property real textPadding: 8
        readonly property real sliderHandleSize: 18
        readonly property real sliderGrooveHeight: 8
        readonly property real spinnerSize: 42
        readonly property real tablePadding: 12
        readonly property real tableRowHeight: largeScreen ? 26 : 23
        readonly property vector2d modalDialogMargin: Qt.vector2d(50, 30)
        readonly property real modalDialogTitleHeight: 40
        readonly property real controlLineHeight: 29  // Height of spinbox control on 1920 x 1080 monitor
        readonly property real controlInterlineHeight: 22  // 75% of controlLineHeight
        readonly property vector2d menuPadding: Qt.vector2d(14, 12)
    }

    Item {
        id: fontSizes  // In pixels
        readonly property real overlayTitle: dimensions.largeScreen ? 18 : 14
        readonly property real tabName: dimensions.largeScreen ? 12 : 10
        readonly property real sectionName: dimensions.largeScreen ? 12 : 10
        readonly property real inputLabel: dimensions.largeScreen ? 14 : 10
        readonly property real textFieldInput: dimensions.largeScreen ? 15 : 12
        readonly property real tableText: dimensions.largeScreen ? 15 : 12
        readonly property real buttonLabel: dimensions.largeScreen ? 13 : 9
        readonly property real iconButton: dimensions.largeScreen ? 13 : 9
        readonly property real listItem: dimensions.largeScreen ? 15 : 11
        readonly property real tabularData: dimensions.largeScreen ? 15 : 11
        readonly property real logs: dimensions.largeScreen ? 16 : 12
        readonly property real code: dimensions.largeScreen ? 16 : 12
        readonly property real rootMenu: dimensions.largeScreen ? 15 : 11
        readonly property real rootMenuDisclosure: dimensions.largeScreen ? 20 : 16
        readonly property real menuItem: dimensions.largeScreen ? 15 : 11
        readonly property real shortcutText: dimensions.largeScreen ? 13 : 9
        readonly property real carat: dimensions.largeScreen ? 38 : 30
        readonly property real disclosureButton: dimensions.largeScreen ? 20 : 15
    }

    Item {
        id: glyphs
        readonly property string alert: "+"
        readonly property string backward: "E"
        readonly property string caratDn: "5"
        readonly property string caratR: "3"
        readonly property string caratUp: "6"
        readonly property string close: "w"
        readonly property string closeInverted: "x"
        readonly property string closeSmall: "C"
        readonly property string critical: "="
        readonly property string disclosureButtonCollapse: "M"
        readonly property string disclosureButtonExpand: "L"
        readonly property string disclosureCollapse: "Z"
        readonly property string disclosureExpand: "B"
        readonly property string forward: "D"
        readonly property string info: "["
        readonly property string noIcon: ""
        readonly property string pin: "y"
        readonly property string pinInverted: "z"
        readonly property string placemark: "U"
        readonly property string question: "]"
        readonly property string reloadSmall: "a"
        readonly property string resizeHandle: "A"
    }

    Item {
        id: icons
        // Values per OffscreenUi::Icon
        readonly property int none: 0
        readonly property int question: 1
        readonly property int information: 2
        readonly property int warning: 3
        readonly property int critical: 4
        readonly property int placemark: 5
    }

    Item {
        id: buttons
        readonly property int white: 0
        readonly property int blue: 1
        readonly property int red: 2
        readonly property int black: 3
        readonly property var textColor: [ colors.darkGray, colors.white, colors.white, colors.white ]
        readonly property var colorStart: [ "#ffffff", "#00b4ef", "#d42043", "#343434" ]
        readonly property var colorFinish: [ "#afafaf", "#1080b8", "#94132e", "#000000" ]
        readonly property int radius: 5
    }

    QtObject {
        id: effects
        readonly property int fadeInDuration: 300
    }
}
