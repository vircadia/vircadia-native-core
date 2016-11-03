//
//  Section.qml
//
//  Created by Bradley Austin Davis on 18 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Hifi 1.0

import "../../controls-uit" as HiFiControls
import "../../styles-uit"
import "."

Preference {
    id: root
    property bool collapsable: true
    property bool expanded: false
    property bool isFirst: false
    property bool isLast: false
    property string name: "Header"
    property real spacing: 8
    default property alias preferences: contentContainer.children

    HifiConstants { id: hifi }

    function saveAll() {
        for (var i = 0; i < d.preferences.length; ++i) {
            var preference = d.preferences[i];
            preference.save();
        }
    }

    function restoreAll() {
        for (var i = 0; i < d.preferences.length; ++i) {
            var preference = d.preferences[i];
            preference.restore();
        }
    }

    children: [ contentContainer ]

    height: contentContainer.height + (contentContainer.isCollapsed ? 0 : hifi.dimensions.controlInterlineHeight)

    Component.onCompleted: d.buildPreferences();

    HiFiControls.ContentSection {
        id: contentContainer
        name: root.name
        isFirst: root.isFirst
        isCollapsible: root.collapsable
        isCollapsed: !root.expanded

        anchors {
            left: parent.left
            right: parent.right
            margins: 0
        }
    }

    QtObject {
        id: d
        property var editableBuilder: Component { EditablePreference { } }
        property var browsableBuilder: Component { BrowsablePreference { } }
        property var spinnerBuilder: Component { SpinBoxPreference { } }
        property var checkboxBuilder: Component { CheckBoxPreference { } }
        property var sliderBuilder: Component { SliderPreference { } }
        property var avatarBuilder: Component { AvatarPreference { } }
        property var buttonBuilder: Component { ButtonPreference { } }
        property var comboBoxBuilder: Component { ComboBoxPreference { } }
        property var preferences: []
        property int checkBoxCount: 0

        function buildPreferences() {
            var categoryPreferences = Preferences.preferencesByCategory[root.name];
            if (categoryPreferences) {
                console.log("Category " + root.name + " with " + categoryPreferences.length + " preferences");
                for (var j = 0; j < categoryPreferences.length; ++j) {
                    buildPreference(categoryPreferences[j]);
                }
            }
        }

        function buildPreference(preference) {
            console.log("\tPreference type " + preference.type + " name " + preference.name)
            var builder;
            switch (preference.type) {
                case Preference.Editable:
                    checkBoxCount = 0;
                    builder = editableBuilder;
                    break;

                case Preference.Browsable:
                    checkBoxCount = 0;
                    builder = browsableBuilder;
                    break;

                case Preference.Spinner:
                    checkBoxCount = 0;
                    builder = spinnerBuilder;
                    break;

                case Preference.Slider:
                    checkBoxCount = 0;
                    builder = sliderBuilder;
                    break;

                case Preference.Checkbox:
                    checkBoxCount++;
                    builder = checkboxBuilder;
                    break;

                case Preference.Avatar:
                    checkBoxCount = 0;
                    builder = avatarBuilder;
                    break;

                case Preference.Button:
                    checkBoxCount = 0;
                    builder = buttonBuilder;
                    break;

                case Preference.ComboBox:
                    checkBoxCount = 0;
                    builder = comboBoxBuilder;
                    break;
            };

            if (builder) {
                preferences.push(builder.createObject(contentContainer, { preference: preference, isFirstCheckBox: (checkBoxCount === 1) }));
            }
        }
    }
}

