import QtQuick 2.0

Item {
    id: keyboardBase
    height: 200
    Rectangle {
        id: leftRect
        y: 0
        height: 200
        color: "#898989"
        anchors.right: keyboardRect.left
        anchors.rightMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
    }

    Rectangle {
        id: keyboardRect
        x: 206
        y: 0
        width: 480
        height: 200
        color: "#b6b6b6"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0

        Column {
            id: column1
            width: 480
            height: 200

            Row {
                id: row1
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 12

                Key {
                    id: key1
                    glyph: "q"
                }

                Key {
                    id: key2
                    glyph: "w"
                }

                Key {
                    id: key3
                    glyph: "e"
                }

                Key {
                    id: key4
                    glyph: "r"
                }

                Key {
                    id: key5
                    glyph: "t"
                }

                Key {
                    id: key6
                    glyph: "y"
                }

                Key {
                    id: key7
                    glyph: "u"
                }

                Key {
                    id: key8
                    glyph: "i"
                }

                Key {
                    id: key9
                    glyph: "o"
                }

                Key {
                    id: key10
                    glyph: "p"
                }
            }

            Row {
                id: row2
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 34

                Key {
                    id: key11
                }

                Key {
                    id: key12
                    glyph: "s"
                }

                Key {
                    id: key13
                    glyph: "d"
                }

                Key {
                    id: key14
                    glyph: "f"
                }

                Key {
                    id: key15
                    glyph: "g"
                }

                Key {
                    id: key16
                    glyph: "h"
                }

                Key {
                    id: key17
                    glyph: "j"
                }

                Key {
                    id: key18
                    glyph: "k"
                }

                Key {
                    id: key19
                    glyph: "l"
                }
            }

            Row {
                id: row3
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 0

                Key {
                    id: key27
                    width: 79
                    glyph: "⇪"
                }

                Key {
                    id: key20
                    glyph: "z"
                }

                Key {
                    id: key21
                    glyph: "x"
                }

                Key {
                    id: key22
                    glyph: "c"
                }

                Key {
                    id: key23
                    glyph: "v"
                }

                Key {
                    id: key24
                    glyph: "b"
                }

                Key {
                    id: key25
                    glyph: "n"
                }

                Key {
                    id: key26
                    glyph: "m"
                }

                Key {
                    id: key28
                    width: 85
                    glyph: "←"
                }

            }

            Row {
                id: row4
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 59

                Key {
                    id: key30
                    width: 45
                    glyph: "⁂"
                }

                Key {
                    id: key29
                    width: 200
                    glyph: " "
                }

                Key {
                    id: key31
                    glyph: "."
                }

                Key {
                    id: key32
                    width: 65
                    glyph: "⏎"
                }

            }
        }
    }

    Rectangle {
        id: rightRect
        y: 280
        height: 200
        color: "#8d8d8d"
        anchors.left: keyboardRect.right
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
    }

    Rectangle {
        id: rectangle1
        color: "#ffffff"
        anchors.bottom: keyboardRect.top
        anchors.bottomMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }

}
