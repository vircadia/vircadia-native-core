import QtQuick 2.5
import QtQuick.Controls 1.4

Preference {
    id: root
    property alias buttonText: button.text
    property alias text: dataTextField.text
    property alias placeholderText: dataTextField.placeholderText
    property real spacing: 8
    property var browser;
    height: labelText.height + Math.max(dataTextField.height, button.height) + spacing

    Component.onCompleted: {
        dataTextField.text = preference.value;
        console.log("MyAvatar modelName " + MyAvatar.getFullAvatarModelName())
        console.log("Application : " + ApplicationInterface)
        ApplicationInterface.fullAvatarURLChanged.connect(processNewAvatar);
    }

    Component.onDestruction: {
        ApplicationInterface.fullAvatarURLChanged.disconnect(processNewAvatar);
    }

    function processNewAvatar(url, modelName) {
        if (browser) {
            browser.destroy();
            browser = null
        }

        dataTextField.text = url;
    }

    function save() {
        preference.value = dataTextField.text;
        preference.save();
    }

    // Restores the original avatar URL
    function restore() {
        preference.save();
    }

    Text {
        id: labelText
        color: enabled ? "black" : "gray"
        text:  root.label
    }

    TextField {
        id: dataTextField
        placeholderText: root.placeholderText
        text: preference.value
        anchors {
            top: labelText.bottom
            left: parent.left
            right: button.left
            topMargin: root.spacing
            rightMargin: root.spacing
        }
    }

    Component {
        id: avatarBrowserBuilder;
        AvatarBrowser { }
    }

    Button {
        id: button
        anchors { right: parent.right; verticalCenter: dataTextField.verticalCenter }
        text: "Browse"
        onClicked: {
            root.browser = avatarBrowserBuilder.createObject(desktop);
            root.browser.windowDestroyed.connect(function(){
                root.browser = null;
            })
        }
    }
}
