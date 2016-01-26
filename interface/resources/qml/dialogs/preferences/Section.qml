import QtQuick 2.5
import QtQuick.Controls 1.4
import Hifi 1.0

import "../../controls" as VrControls
import "."

Preference {
    id: root
    property bool expanded: false
    property string name: "Header"
    property real spacing: 8
    readonly property alias toggle: toggle
    readonly property alias header: header
    default property alias preferences: contentContainer.children

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

    clip: true
    children: [ toggle, header, contentContainer ]
    height: expanded ? header.height + contentContainer.height + root.spacing * 3
                     : Math.max(toggle.height, header.height) + root.spacing * 2
    Behavior on height { PropertyAnimation {} }

    Component.onCompleted: d.buildPreferences();

    function toggleExpanded() {
        root.expanded = !root.expanded;
    }

    VrControls.FontAwesome {
        id: toggle
        anchors { left: parent.left; top: parent.top; margins: root.spacing }
        rotation: root.expanded ? 0 : -90
        text: "\uf078"
        Behavior on rotation { PropertyAnimation {} }
        MouseArea { anchors.fill: parent; onClicked: root.toggleExpanded() }
    }

    Text {
        id: header
        anchors { left: toggle.right; top: parent.top; leftMargin: root.spacing * 2; margins: root.spacing }
        font.bold: true
        font.pointSize: 16
        color: "#0e7077"
        text: root.name
        MouseArea { anchors.fill: parent; onClicked: root.toggleExpanded() }
    }

    Column {
        id: contentContainer
        spacing: root.spacing
        anchors { left: toggle.right; top: header.bottom; topMargin: root.spacing; right: parent.right; margins: root.spacing }
        enabled: root.expanded
        visible: root.expanded
        clip: true
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
        property var preferences: []

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
                    builder = editableBuilder;
                    break;

                case Preference.Browsable:
                    builder = browsableBuilder;
                    break;

                case Preference.Spinner:
                    builder = spinnerBuilder;
                    break;

                case Preference.Slider:
                    builder = sliderBuilder;
                    break;

                case Preference.Checkbox:
                    builder = checkboxBuilder;
                    break;

                case Preference.Avatar:
                    builder = avatarBuilder;
                    break;

                case Preference.Button:
                    builder = buttonBuilder;
                    break
            };

            if (builder) {
                preferences.push(builder.createObject(contentContainer, { preference: preference }));
            }
        }
    }
}

