ButtonStyle {
    background:  Item { anchors.fill: parent }
    label: FontAwesome {
        id: icon
        font.pointSize: 18
        property alias unicode: text
        text: control.text
        color: control.enabled ? hifi.colors.text : hifi.colors.disabledText
    }
}
