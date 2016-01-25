import QtQuick 2.0
import QtQuick.Controls 1.4

TableView {
    id: root

    itemDelegate: Component {
        Item {
            clip: true
            Text {
                x: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.activeFocus && styleData.row === root.currentRow ? "yellow" : styleData.textColor
                elide: styleData.elideMode
                text: getText();
                font.italic: root.model.get(styleData.row, "fileIsDir") ? true : false

                function getText() {
                    switch (styleData.column) {
                        //case 1: return Date.fromLocaleString(locale, styleData.value, "yyyy-MM-dd hh:mm:ss");
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
        role: "fileName"
        title: "Name"
        width: 400
    }
    TableViewColumn {
        role: "fileModified"
        title: "Date Modified"
        width: 200
    }
    TableViewColumn {
        role: "fileSize"
        title: "Size"
        width: 200
    }
}


