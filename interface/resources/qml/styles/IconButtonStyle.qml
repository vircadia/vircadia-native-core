ButtonStyle {
    background:  Item { anchors.fill: parent }
    label: Text {
        id: icon
        width: height
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
        font.family: iconFont.name
        font.pointSize: 18
        property alias unicode: icon.text
        FontLoader { id: iconFont; source: "../../fonts/fontawesome-webfont.ttf"; }
        text: control.text
        color: control.enabled ? "white" : "dimgray"
    }
}
