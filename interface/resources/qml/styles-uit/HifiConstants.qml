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
                glyph = hifi.glyphs.error;
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
        readonly property color blueHighlight: "#00b4ef"
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
        readonly property color locked: "#252525"
        // Semitransparent
        readonly property color white50: "#80ffffff"
        readonly property color white30: "#4dffffff"
        readonly property color white25: "#40ffffff"
        readonly property color transparent: "#00ffffff"

        // Control specific colors
        readonly property color tableRowLightOdd: "#fafafa"
        readonly property color tableRowLightEven: "#eeeeee" // Equivavlent to "#1a575757" over #e3e3e3 background
        readonly property color tableRowDarkOdd: "#2e2e2e"   // Equivalent to "#80393939" over #404040 background
        readonly property color tableRowDarkEven: "#1c1c1c"  // Equivalent to "#a6181818" over #404040 background
        readonly property color tableBackgroundLight: tableRowLightEven
        readonly property color tableBackgroundDark: tableRowDarkEven
        readonly property color tableScrollHandleLight: "#8F8F8F"
        readonly property color tableScrollHandleDark: "#707070"
        readonly property color tableScrollBackgroundLight: tableRowLightEven
        readonly property color tableScrollBackgroundDark: "#323232"
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
        readonly property color textFieldLightBackground: "#d4d4d4"
        readonly property color tabBackgroundDark: "#252525"
        readonly property color tabBackgroundLight: "#d4d4d4"
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
        readonly property vector2d contentMargin: Qt.vector2d(21, 21)
        readonly property vector2d contentSpacing: Qt.vector2d(11, 14)
        readonly property real labelPadding: 40
        readonly property real textPadding: 8
        readonly property real sliderHandleSize: 18
        readonly property real sliderGrooveHeight: 8
        readonly property real frameIconSize: 22
        readonly property real spinnerSize: 50
        readonly property real tablePadding: 12
        readonly property real tableRowHeight: largeScreen ? 26 : 23
        readonly property real tableHeaderHeight: 29
        readonly property vector2d modalDialogMargin: Qt.vector2d(50, 30)
        readonly property real modalDialogTitleHeight: 40
        readonly property real controlLineHeight: 28  // Height of spinbox control on 1920 x 1080 monitor
        readonly property real controlInterlineHeight: 21  // 75% of controlLineHeight
        readonly property vector2d menuPadding: Qt.vector2d(14, 12)
    }

    Item {
        id: fontSizes  // In pixels
        readonly property real overlayTitle: dimensions.largeScreen ? 18 : 14
        readonly property real tabName: dimensions.largeScreen ? 12 : 10
        readonly property real sectionName: dimensions.largeScreen ? 12 : 10
        readonly property real inputLabel: dimensions.largeScreen ? 14 : 10
        readonly property real textFieldInput: dimensions.largeScreen ? 15 : 12
        readonly property real textFieldInputLabel: dimensions.largeScreen ? 13 : 9
        readonly property real textFieldSearchIcon: dimensions.largeScreen ? 30 : 24
        readonly property real tableHeading: dimensions.largeScreen ? 12 : 10
        readonly property real tableHeadingIcon: dimensions.largeScreen ? 40 : 33
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
        readonly property real disclosureButton: dimensions.largeScreen ? 30 : 22
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
        readonly property var colorStart: [ colors.white, colors.primaryHighlight, "#d42043", "#343434" ]
        readonly property var colorFinish: [ colors.lightGrayText, colors.blueAccent, "#94132e", colors.black ]
        readonly property var hoveredColor: [ colorStart[white], colorStart[blue], colorStart[red], colorFinish[black] ]
        readonly property var pressedColor: [ colorFinish[white], colorFinish[blue], colorFinish[red], colorStart[black] ]
        readonly property var disabledColorStart: [ colorStart[white], colors.baseGrayHighlight]
        readonly property var disabledColorFinish: [ colorFinish[white], colors.baseGrayShadow]
        readonly property var disabledTextColor: [ colors.lightGrayText, colors.baseGrayShadow]
        readonly property int radius: 5
    }

    QtObject {
        id: effects
        readonly property int fadeInDuration: 300
    }
    Item {
        id: glyphs
        readonly property string noIcon: ""
        readonly property string hmd: "b"
        readonly property string screen: "c"
        readonly property string keyboard: "d"
        readonly property string handControllers: "e"
        readonly property string headphonesMic: "f"
        readonly property string gamepad: "g"
        readonly property string headphones: "h"
        readonly property string mic: "i"
        readonly property string upload: "j"
        readonly property string script: "k"
        readonly property string text: "l"
        readonly property string cube: "m"
        readonly property string sphere: "n"
        readonly property string zone: "o"
        readonly property string light: "p"
        readonly property string web: "q"
        readonly property string web2: "r"
        readonly property string edit: "s"
        readonly property string market: "t"
        readonly property string directory: "u"
        readonly property string menu: "v"
        readonly property string close: "w"
        readonly property string closeInverted: "x"
        readonly property string pin: "y"
        readonly property string pinInverted: "z"
        readonly property string resizeHandle: "A"
        readonly property string disclosureExpand: "B"
        readonly property string reloadSmall: "a"
        readonly property string closeSmall: "C"
        readonly property string forward: "D"
        readonly property string backward: "E"
        readonly property string reload: "F"
        readonly property string unmuted: "G"
        readonly property string muted: "H"
        readonly property string minimize: "I"
        readonly property string maximize: "J"
        readonly property string maximizeInverted: "K"
        readonly property string disclosureButtonExpand: "L"
        readonly property string disclosureButtonCollapse: "M"
        readonly property string scriptStop: "N"
        readonly property string scriptReload: "O"
        readonly property string scriptRun: "P"
        readonly property string scriptNew: "Q"
        readonly property string hifiForum: "2"
        readonly property string hifiLogoSmall: "S"
        readonly property string avatar1: "T"
        readonly property string placemark: "U"
        readonly property string box: "V"
        readonly property string community: "0"
        readonly property string grabHandle: "X"
        readonly property string search: "Y"
        readonly property string disclosureCollapse: "Z"
        readonly property string scriptUpload: "R"
        readonly property string code: "W"
        readonly property string avatar: "<"
        readonly property string arrowsH: ":"
        readonly property string arrowsV: ";"
        readonly property string arrows: "`"
        readonly property string compress: "!"
        readonly property string expand: "\""
        readonly property string placemark1: "#"
        readonly property string circle: "$"
        readonly property string handPointer: "9"
        readonly property string plusSquareO: "%"
        readonly property string sliders: "&"
        readonly property string square: "'"
        readonly property string alignCenter: "8"
        readonly property string alignJustify: ")"
        readonly property string alignLeft: "*"
        readonly property string alignRight: "^"
        readonly property string bars: "7"
        readonly property string circleSlash: ","
        readonly property string sync: "()"
        readonly property string key: "-"
        readonly property string link: "."
        readonly property string location: "/"
        readonly property string caratR: "3"
        readonly property string caratL: "4"
        readonly property string caratDn: "5"
        readonly property string caratUp: "6"
        readonly property string folderLg: ">"
        readonly property string folderSm: "?"
        readonly property string levelUp: "1"
        readonly property string info: "["
        readonly property string question: "]"
        readonly property string alert: "+"
        readonly property string home: "_"
        readonly property string error: "="
        readonly property string settings: "@"
        readonly property string trash: "{"
        readonly property string objectGroup: "&#xe000;"
        readonly property string cm: "}"
        readonly property string msvg79: "~"
        readonly property string deg: "\\"
        readonly property string px: "|"
    }
}
