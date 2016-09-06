import QtQuick 2.0

Item {
    id: keyboardBase
    height: 200

    function toUpper(str) {
        if (str === ",") {
            return "<";
        } else if (str === ".") {
            return ">";
        } else if (str === "/") {
            return "?";
        } else {
            return str.toUpperCase(str);
        }
    }

    function toLower(str) {
        if (str === "<") {
            return ",";
        } else if (str === ">") {
            return ".";
        } else if (str === "?") {
            return "/";
        } else {
            return str.toLowerCase(str);
        }
    }

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
                    width: 44
                    glyph: "q"
                }

                Key {
                    id: key2
                    width: 44
                    glyph: "w"
                }

                Key {
                    id: key3
                    width: 44
                    glyph: "e"
                }

                Key {
                    id: key4
                    width: 43
                    glyph: "r"
                }

                Key {
                    id: key5
                    width: 43
                    glyph: "t"
                }

                Key {
                    id: key6
                    width: 44
                    glyph: "y"
                }

                Key {
                    id: key7
                    width: 44
                    glyph: "u"
                }

                Key {
                    id: key8
                    width: 43
                    glyph: "i"
                }

                Key {
                    id: key9
                    width: 42
                    glyph: "o"
                }

                Key {
                    id: key10
                    width: 44
                    glyph: "p"
                }

                Key {
                    id: key28
                    width: 45
                    glyph: "←"
                }
            }

            Row {
                id: row2
                width: 480
                height: 50
                anchors.left: parent.left
                anchors.leftMargin: 18

                Key {
                    id: key11
                    width: 43
                }

                Key {
                    id: key12
                    width: 43
                    glyph: "s"
                }

                Key {
                    id: key13
                    width: 43
                    glyph: "d"
                }

                Key {
                    id: key14
                    width: 43
                    glyph: "f"
                }

                Key {
                    id: key15
                    width: 43
                    glyph: "g"
                }

                Key {
                    id: key16
                    width: 43
                    glyph: "h"
                }

                Key {
                    id: key17
                    width: 43
                    glyph: "j"
                }

                Key {
                    id: key18
                    width: 43
                    glyph: "k"
                }

                Key {
                    id: key19
                    width: 43
                    glyph: "l"
                }

                Key {
                    id: key32
                    width: 75
                    glyph: "⏎"
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
                    width: 46
                    glyph: "⇪"
                    toggle: true
                    onToggledChanged: {
                        var i, j;
                        for (i = 0; i < column1.children.length; i++) {
                            var row = column1.children[i];
                            for (j = 0; j < row.children.length; j++) {
                                var key = row.children[j];
                                if (toggled) {
                                    key.glyph = keyboardBase.toUpper(key.glyph);
                                } else {
                                    key.glyph = keyboardBase.toLower(key.glyph);
                                }
                            }
                        }
                    }
                }

                Key {
                    id: key20
                    width: 43
                    glyph: "z"
                }

                Key {
                    id: key21
                    width: 43
                    glyph: "x"
                }

                Key {
                    id: key22
                    width: 43
                    glyph: "c"
                }

                Key {
                    id: key23
                    width: 43
                    glyph: "v"
                }

                Key {
                    id: key24
                    width: 43
                    glyph: "b"
                }

                Key {
                    id: key25
                    width: 43
                    glyph: "n"
                }

                Key {
                    id: key26
                    width: 44
                    glyph: "m"
                }

                Key {
                    id: key31
                    width: 43
                    glyph: ","
                }

                Key {
                    id: key33
                    width: 43
                    glyph: "."
                }

                Key {
                    id: key36
                    width: 46
                    glyph: "/"
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
                    width: 44
                    glyph: "⁂"
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
                    antialiasing: false
                    scale: 1
                    transformOrigin: Item.Center
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
