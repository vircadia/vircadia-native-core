import QtQuick 2.9

ShadowGlyph {
    id: indicator
    property bool isPrevious: true;
    property bool hasNext: false
    property bool hasPrev: false
    property bool isEnabled: isPrevious ? hasPrev : hasNext
    signal clicked;

    states: [
        State {
            name: "hovered"
            when: pageIndicatorMouseArea.containsMouse;
            PropertyChanges { target: pageIndicatorMouseArea; anchors.bottomMargin: -5 }
            PropertyChanges { target: indicator; y: -5 }
            PropertyChanges { target: indicator; dropShadowVerticalOffset: 9 }
        }
    ]

    Behavior on y {
        NumberAnimation {
            duration: 100
        }
    }

    text: isPrevious ? "E" : "D";
    width: 40
    height: 40
    font.pixelSize: 100
    color: isEnabled ? 'black' : 'gray'
    visible: hasNext || hasPrev
    horizontalAlignment: Text.AlignHCenter

    MouseArea {
        id: pageIndicatorMouseArea
        anchors.fill: parent
        enabled: isEnabled
        hoverEnabled: enabled
        onClicked: {
            parent.clicked();
        }
    }
}
