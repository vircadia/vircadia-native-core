//
//  Section.qml
//
//  Created by Bradley Dante Ruiz on 13 Feb 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Hifi 1.0

import "../../../../dialogs/preferences"
import "../../../../controls-uit" as HiFiControls
import "../../../../styles-uit"
import "."

Preference {
    id: root
    property bool collapsable: false
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

    function getPreferencesHeight() {
        var height = 0;
        for (var index = 0; index < d.preferences.length; index++) {
            height += d.preferences[index].height;
        }

        return height;
    }
    children: [ contentContainer ]

    height: contentContainer.height + (contentContainer.isCollapsed ? 0 : hifi.dimensions.controlInterlineHeight)

    Component.onCompleted: d.buildPreferences();

    HiFiControls.TabletContentSection {
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
        property var browsableBuilder: Component { TabletBrowsablePreference { } }
        property var spinnerBuilder: Component { SpinBoxPreference { } }
        property var checkboxBuilder: Component { CheckBoxPreference { } }
        property var sliderBuilder: Component { SliderPreference { } }
        property var avatarBuilder: Component { AvatarPreference { } }
        property var buttonBuilder: Component { ButtonPreference { } }
        property var comboBoxBuilder: Component { ComboBoxPreference { } }
        property var spinnerSliderBuilder: Component { SpinnerSliderPreference { } }
        property var primaryHandBuilder: Component { PrimaryHandPreference { } }
        property var preferences: []
        property int checkBoxCount: 0

        function buildPreferences() {
            var categoryPreferences = Preferences.preferencesByCategory[root.name];
            if (categoryPreferences) {
                console.log("Category " + root.name + " with " + categoryPreferences.length + " preferences");
                for (var j = 0; j < categoryPreferences.length; ++j) {
                    //provide component position within column
                    //lowest numbers on top
                    buildPreference(categoryPreferences[j], j);
                }
            }
        }

        function buildPreference(preference, itemNum) {
            console.log("\tPreference type " + preference.type + " name " + preference.name)
            var builder;
            var zpos;
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
                    //make sure that combo boxes sitting higher will have higher z coordinate
                    //to be not overlapped when drop down is active
                    zpos = root.z + 1000 - itemNum
                    break;

                case Preference.SpinnerSlider:
                    checkBoxCount = 0;
                    builder = spinnerSliderBuilder;
                    break;

                case Preference.PrimaryHand:
                    checkBoxCount++;
                    builder = primaryHandBuilder;
                    break;
            };

            if (builder) {
                preferences.push(builder.createObject(contentContainer, { preference: preference, isFirstCheckBox: (checkBoxCount === 1) , z: zpos}));
            }
        }
    }
}

