import QtQuick 2.5

import "../../controls" as VrControls

VrControls.ComboBox {
    id: root
    property string filtersString:  "All Files (*.*)";
    property var currentFilter: [ "*.*" ];

    // Per http://doc.qt.io/qt-5/qfiledialog.html#getOpenFileName the string can contain
    // multiple filters separated by semicolons
    // ex:  "Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"
    model: filtersString.split(';;');

    enabled: model.length > 1

    onCurrentTextChanged: {
        var globRegex = /\((.*)\)$/
        var globs = globRegex.exec(currentText);
        if (!globs || !globs[1]) {
            globRegex = /^(\*.*)$/
            globs = globRegex.exec(currentText);
            if (!globs || !globs[1]) {
                console.warn("Unable to parse filter " + currentText);
                return;
            }
        }
        currentFilter = globs[1].split(" ");
    }
}

