import QtQuick 2.0

Item {
    id: keyboardBase
    height: 200
    Rectangle {
        id: leftRect
        y: 0
        height: 200
        color: "#252525"
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
        color: "#252525"
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
                anchors.leftMargin: 0

                Key {
                    id: key1
                    width: 43
                    glyph: "1"
                }

                Key {
                    id: key2
                    width: 43
                    glyph: "2"
                }

                Key {
                    id: key3
                    width: 43
                    glyph: "3"
                }

                Key {
                    id: key4
                    width: 43
                    glyph: "4"
                }

                Key {
                    id: key5
                    width: 43
                    glyph: "5"
                }

                Key {
                    id: key6
                    width: 43
                    glyph: "6"
                }

                Key {
                    id: key7
                    width: 43
                    glyph: "7"
                }

                Key {
                    id: key8
                    width: 43
                    glyph: "8"
                }

                Key {
                    id: key9
                    width: 43
                    glyph: "9"
                }

                Key {
                    id: key10
                    width: 43
                    glyph: "0"
                }

                Key {
                    id: key28
                    width: 50
                    glyph: "←"
                }
            }

            Row {
                id: row2
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 0

                Key {
                    id: key11
                    width: 43
                    glyph: "!"
                }

                Key {
                    id: key12
                    width: 43
                    glyph: "@"
                }

                Key {
                    id: key13
                    width: 43
                    glyph: "#"
                }

                Key {
                    id: key14
                    width: 43
                    glyph: "$"
                }

                Key {
                    id: key15
                    width: 43
                    glyph: "%"
                }

                Key {
                    id: key16
                    width: 43
                    glyph: "^"
                }

                Key {
                    id: key17
                    width: 43
                    glyph: "&"
                }

                Key {
                    id: key18
                    width: 43
                    glyph: "*"
                }

                Key {
                    id: key19
                    width: 43
                    glyph: "("
                }

                Key {
                    id: key32
                    width: 43
                    glyph: ")"
                }

                Key {
                    id: key37
                    width: 50
                    glyph: "⏎"
                }
            }

            Row {
                id: row3
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 4

                Key {
                    id: key27
                    width: 43
                    glyph: "="
                }

                Key {
                    id: key20
                    width: 43
                    glyph: "+"
                }

                Key {
                    id: key21
                    width: 43
                    glyph: "-"
                }

                Key {
                    id: key22
                    width: 43
                    glyph: "_"
                }

                Key {
                    id: key23
                    width: 43
                    glyph: ";"
                }

                Key {
                    id: key24
                    width: 43
                    glyph: ":"
                }

                Key {
                    id: key25
                    width: 43
                    glyph: "'"
                }

                Key {
                    id: key26
                    width: 43
                    glyph: "\""
                }

                Key {
                    id: key31
                    width: 43
                    glyph: "<"
                }

                Key {
                    id: key33
                    width: 43
                    glyph: ">"
                }

                Key {
                    id: key36
                    width: 43
                    glyph: "?"
                }

            }

            Row {
                id: row4
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 19

                Key {
                    id: key30
                    width: 65
                    glyph: "abc"
                    mouseArea.onClicked: {
                        keyboardBase.parent.punctuationMode = false
                    }
                }

                Key {
                    id: key29
                    width: 285
                    glyph: " "
                }

                Key {
                    id: key34
                    width: 43
                    glyph: "⇦"
                }

                Key {
                    id: key35
                    x: 343
                    width: 43
                    glyph: "⇨"
                }

            }
        }
    }

    Rectangle {
        id: rightRect
        y: 280
        height: 200
        color: "#252525"
        border.width: 0
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
