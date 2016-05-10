//
//  FileDialog.qml
//
//  Created by Bradley Austin Davis on 22 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

TableView {
    id: root
    onActiveFocusChanged:  {
        if (activeFocus && currentRow == -1) {
            root.selection.select(0)
        }
    }
    //horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

    itemDelegate: Component {
        Item {
            clip: true
            Text {
                x: 3
                anchors.verticalCenter: parent.verticalCenter
                color: styleData.textColor
                elide: styleData.elideMode
                text: getText();
                font.italic: root.model.get(styleData.row, "fileIsDir") ? true : false

                function getText() {
                    switch (styleData.column) {
                        //case 1: return Date.fromLocaleString(locale, styleData.value, "yyyy-MM-dd hh:mm:ss");
                        case 1: return root.model.get(styleData.row, "fileIsDir") ? "" : styleData.value;
                        case 2: return root.model.get(styleData.row, "fileIsDir") ? "" : formatSize(styleData.value);
                        default: return styleData.value;
                    }
                }
                function formatSize(size) {
                    var suffixes = [ "bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" ];
                    var suffixIndex = 0
                    while ((size / 1024.0) > 1.1) {
                        size /= 1024.0;
                        ++suffixIndex;
                    }

                    size = Math.round(size*1000)/1000;
                    size = size.toLocaleString()

                    return size + " " + suffixes[suffixIndex];
                }
            }
        }
    }

    TableViewColumn {
        id: fileNameColumn
        role: "fileName"
        title: "Name"
        width: Math.floor(0.55 * parent.width)
        resizable: true
    }
    TableViewColumn {
        id: fileMofifiedColumn
        role: "fileModified"
        title: "Date"
        width: Math.floor(0.3 * parent.width)
        resizable: true
    }
    TableViewColumn {
        role: "fileSize"
        title: "Size"
        width: Math.floor(0.15 * parent.width) - 16 - 2  // Allow space for vertical scrollbar and borders
        resizable: true
    }
}
