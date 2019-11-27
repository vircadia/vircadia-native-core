//
//  SimplifiedConstants.qml
//
//  Created by Zach Fox on 2019-05-02
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
QtObject {
    readonly property QtObject colors: QtObject {
        readonly property QtObject text: QtObject {
            readonly property color almostWhite: "#FAFAFA"
            readonly property color lightGrey: "#CCCCCC"
            readonly property color darkGrey: "#8F8F8F"
            readonly property color white: "#FFFFFF"
            readonly property color lightBlue: "#00B4EF"
            readonly property color lightBlueHover: "#3dcfff"
            readonly property color black: "#000000"
        }

        readonly property QtObject controls: QtObject {
            readonly property QtObject radioButton: QtObject {
                readonly property QtObject background: QtObject {
                    readonly property color startColor: "#828282"
                    readonly property real startPosition: 0.15
                    readonly property color endColor: "#6A6A6A"
                    readonly property real endPosition: 1.0
                    readonly property real disabledOpacity: 0.5
                }
                readonly property QtObject hover: QtObject {
                    readonly property color outerBorderColor: "#00B4EF"
                    readonly property color innerColor: "#00B4EF"
                    readonly property color innerBorderColor: "#36CDFF"
                    readonly property real innerOpacity: 0.5
                }
                readonly property QtObject active: QtObject {
                    readonly property color color: "#00B4EF"
                }
                readonly property QtObject checked: QtObject {
                    readonly property color innerColor: "#00B4EF"
                    readonly property color innerBorderColor: "#36CDFF"
                }
            }
            readonly property QtObject slider: QtObject {
                readonly property QtObject background: QtObject {
                    readonly property color empty: "#252525"
                    readonly property QtObject filled: QtObject {
                        readonly property color start: "#0093C5"
                        readonly property color finish: "#00B4EF"
                    }
                }
                readonly property QtObject handle: QtObject {
                    readonly property color disabledBorder: "#2A2A2A"
                    readonly property color enabledBorder: "#00B4EF"
                    readonly property QtObject disabled: QtObject {
                        readonly property color start: "#2A2A2A"
                        readonly property color finish: "#2A2A2A"
                    }
                    readonly property QtObject normal: QtObject {
                        readonly property color start: "#828282"
                        readonly property color finish: "#6A6A6A"
                    }
                    readonly property QtObject hover: QtObject {
                        readonly property color start: "#48C7F4"
                        readonly property color finish: "#48C7F4"
                    }
                    readonly property QtObject pressed: QtObject {
                        readonly property color start: "#48C7F4"
                        readonly property color finish: "#48C7F4"
                        readonly property color border: "#00B4EF"
                    }
                }
                readonly property QtObject text: QtObject {
                    readonly property color enabled: "#FFFFFF"
                    readonly property color disabled: "#8F8F8F"
                }
            }
            readonly property QtObject simplifiedSwitch: QtObject {
                readonly property QtObject background: QtObject {
                    readonly property color disabled: "#2B2B2B"
                    readonly property color off: "#616161"
                    readonly property color hover: "#616161"
                    readonly property color pressed: "#616161"
                    readonly property color on: "#ffffff"
                }
                readonly property QtObject handle: QtObject {
                    readonly property color disabled: "#616161"
                    readonly property color off: "#E6E6E6"
                    readonly property color hover: "#48C7F4"
                    readonly property color active: "#48C7F4"
                    readonly property color activeBorder: "#00B4EF"
                    readonly property color on: "#00B4EF"
                    readonly property color checkedBorder: "#36CDFF"
                }
                readonly property QtObject text: QtObject {
                    readonly property color off: "#8F8F8F"
                    readonly property color on: "#ffffff"
                }
            }
            readonly property QtObject button: QtObject {
                readonly property QtObject background: QtObject {
                    readonly property color disabled: "#191919"
                    readonly property color enabled: "#191919"
                    readonly property color hover: "#00B4EF"
                    readonly property color active: "#00B4EF"
                }
                readonly property QtObject border: QtObject {
                    readonly property color disabled: "#8F8F8F"
                    readonly property color enabled: "#FFFFFF"
                    readonly property color hover: "#FFFFFF"
                    readonly property color active: "#FFFFFF"
                }
                readonly property QtObject text: QtObject {
                    readonly property color disabled: "#8F8F8F"
                    readonly property color enabled: "#FFFFFF"
                }
            }
            readonly property QtObject outputVolumeButton: QtObject {
                readonly property QtObject text: QtObject {
                    readonly property color muted: "#b20012"
                    readonly property color noisy: "#FFFFFF"
                }
            }
            readonly property QtObject inputVolumeButton: QtObject {
                readonly property QtObject text: QtObject {
                    readonly property color muted: "#b20012"
                    readonly property color noisy: "#FFFFFF"
                }
            }
            readonly property QtObject checkBox: QtObject {
                readonly property QtObject background: QtObject {
                    readonly property color disabled: "#464646"
                    readonly property color active: "#00B4EF"
                    readonly property color enabled: "#767676"
                }
                readonly property QtObject border: QtObject {
                    readonly property color hover: "#00B4EF"
                }
                readonly property QtObject innerBox: QtObject {
                    readonly property color border: "#36CDFF"
                    readonly property color background: "#00B4EF"
                }
            }
            readonly property QtObject textField: QtObject {
                readonly property color normal: Qt.rgba(1, 1, 1, 0.3)
                readonly property color hover: "#FFFFFF"
                readonly property color focus: "#FFFFFF"
            }
            readonly property QtObject scrollBar: QtObject {
                readonly property color background: "#474747"
                readonly property color contentItem: "#0198CB"
            }
            readonly property QtObject table: QtObject {
                readonly property color cellBackground: "#000000"
                readonly property color textColor: "#ffffff"
            }
        }

        readonly property color darkSeparator: "#595959"
        readonly property color darkBackground: "#000000"
        readonly property color darkBackgroundHighlight: "#575757"
        readonly property color highlightOnDark: Qt.rgba(1, 1, 1, 0.2)
        readonly property color white: "#FFFFFF"
    }

    readonly property QtObject glyphs: QtObject {
        readonly property string gear: "@"
        readonly property string editPencil: "\ue00d"
        readonly property string playback_play: "\ue01d"
        readonly property string stop_square: "\ue01e"
        readonly property string hmd: "b"
        readonly property string screen: "c"
        readonly property string vol_0: "\ue00e"
        readonly property string vol_1: "\ue00f"
        readonly property string vol_2: "\ue010"
        readonly property string vol_3: "\ue011"
        readonly property string vol_4: "\ue012"
        readonly property string vol_x_0: "\ue013"
        readonly property string vol_x_1: "\ue014"
        readonly property string vol_x_2: "\ue015"
        readonly property string vol_x_3: "\ue016"
        readonly property string vol_x_4: "\ue017"
        readonly property string muted: "H"
        readonly property string pencil: "\ue00d"
    }

    readonly property QtObject margins: QtObject {
        readonly property QtObject controls: QtObject {
            readonly property QtObject radioButton: QtObject {
                readonly property int labelLeftMargin: 6
            }
        }

        readonly property QtObject settings: QtObject {
            property int subtitleTopMargin: 2
            property int settingsGroupTopMargin: 14
            property int spacingBetweenSettings: 48
            property int spacingBetweenRadiobuttons: 14
        }
    }

    readonly property QtObject sizes: QtObject {
        readonly property QtObject controls: QtObject {
            readonly property QtObject slider: QtObject {
                readonly property int height: 16
                readonly property int labelTextSize: 14
                readonly property int backgroundHeight: 8
            }
            readonly property QtObject radioButton: QtObject {
                readonly property int outerBorderWidth: 1
                readonly property int innerBorderWidth: 1
            }
            readonly property QtObject simplifiedSwitch: QtObject {
                readonly property int switchBackgroundHeight: 8
                readonly property int switchBackgroundWidth: 30
                readonly property int switchHandleInnerWidth: 12
                readonly property int switchHandleOuterWidth: 16
                readonly property int switchHandleBorderSize: 1
                readonly property int labelTextSize: 14
                readonly property int labelGlyphSize: 32
            }
            readonly property QtObject button: QtObject {
                readonly property int borderWidth: 1
                readonly property int textPadding: 16
                readonly property int width: 160
                readonly property int height: 32
                readonly property int textSize: 14
            }
            readonly property QtObject textField: QtObject {
                readonly property int rightGlyphPadding: 6
            }
            readonly property QtObject scrollBar: QtObject {
                readonly property int backgroundWidth: 9
                readonly property int contentItemWidth: 7
            }
        }
    }

    readonly property QtObject numericConstants: QtObject {
        readonly property real mutedValue: -60.0
    }
}
