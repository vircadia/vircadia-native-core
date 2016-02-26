//
//  BrowsablePreference.qml
//
//  Created by Bradley Austin Davis on 18 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../../dialogs"
import "../../controls-uit"

Preference {
    id: root
    property alias text: dataTextField.text
    property alias placeholderText: dataTextField.placeholderText
    property real spacing: 0
    height: Math.max(dataTextField.controlHeight, button.height) + spacing

    Component.onCompleted: {
        dataTextField.text = preference.value;
    }

    function save() {
        preference.value = dataTextField.text;
        preference.save();
    }

    TextField {
        id: dataTextField

        anchors {
            left: parent.left
            right: button.left
            rightMargin: hifi.dimensions.contentSpacing.x
            bottomMargin: spacing
        }

        label: root.label
        placeholderText: root.placeholderText
        colorScheme: hifi.colorSchemes.dark
    }

    Component {
        id: fileBrowserBuilder;
        FileDialog { selectDirectory: true }
    }

    Button {
        id: button
        anchors { right: parent.right; verticalCenter: dataTextField.verticalCenter }
        text: preference.browseLabel
        onClicked: {
            var browser = fileBrowserBuilder.createObject(desktop, { selectDirectory: true, folder: fileDialogHelper.pathToUrl(preference.value) });
            browser.selectedFile.connect(function(fileUrl){
                console.log(fileUrl);
                dataTextField.text = fileDialogHelper.urlToPath(fileUrl);
            });
        }
    }
}
