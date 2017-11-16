import QtQuick 2.7

Shortcut {
    id: root
    property string text
    property alias shortcut: root.sequence
    signal triggered()
}
