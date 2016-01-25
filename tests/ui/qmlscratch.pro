TEMPLATE = app

QT += gui qml quick xml webengine widgets

CONFIG += c++11

SOURCES += src/main.cpp \
    ../../libraries/ui/src/FileDialogHelper.cpp

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

DISTFILES += \
    qml/UI.js \
    qml/main.qml \
    qml/Palettes.qml \
    qml/StubMenu.qml \
    qml/Stubs.qml \
    qml/TestControllers.qml \
    qml/TestDialog.qml \
    qml/TestMenu.qml \
    qml/TestRoot.qml \
    qml/controlDemo/ButtonPage.qml \
    qml/controlDemo/InputPage.qml \
    qml/controlDemo/main.qml \
    qml/controlDemo/ProgressPage.qml \
    ../../interface/resources/qml/controller/hydra/HydraButtons.qml \
    ../../interface/resources/qml/controller/hydra/HydraStick.qml \
    ../../interface/resources/qml/controller/xbox/DPad.qml \
    ../../interface/resources/qml/controller/xbox/LeftAnalogStick.qml \
    ../../interface/resources/qml/controller/xbox/RightAnalogStick.qml \
    ../../interface/resources/qml/controller/xbox/XboxButtons.qml \
    ../../interface/resources/qml/controller/AnalogButton.qml \
    ../../interface/resources/qml/controller/AnalogStick.qml \
    ../../interface/resources/qml/controller/Hydra.qml \
    ../../interface/resources/qml/controller/Standard.qml \
    ../../interface/resources/qml/controller/ToggleButton.qml \
    ../../interface/resources/qml/controller/Xbox.qml \
    ../../interface/resources/qml/controls/Button.qml \
    ../../interface/resources/qml/controls/ButtonAwesome.qml \
    ../../interface/resources/qml/controls/CheckBox.qml \
    ../../interface/resources/qml/controls/FontAwesome.qml \
    ../../interface/resources/qml/controls/Player.qml \
    ../../interface/resources/qml/controls/Slider.qml \
    ../../interface/resources/qml/controls/Spacer.qml \
    ../../interface/resources/qml/controls/SpinBox.qml \
    ../../interface/resources/qml/controls/Text.qml \
    ../../interface/resources/qml/controls/TextAndSlider.qml \
    ../../interface/resources/qml/controls/TextAndSpinBox.qml \
    ../../interface/resources/qml/controls/TextArea.qml \
    ../../interface/resources/qml/controls/TextEdit.qml \
    ../../interface/resources/qml/controls/TextHeader.qml \
    ../../interface/resources/qml/controls/TextInput.qml \
    ../../interface/resources/qml/controls/TextInputAndButton.qml \
    ../../interface/resources/qml/controls/WebView.qml \
    ../../interface/resources/qml/dialogs/FileDialog.qml \
    ../../interface/resources/qml/dialogs/MessageDialog.qml \
    ../../interface/resources/qml/dialogs/RunningScripts.qml \
    ../../interface/resources/qml/hifi/MenuOption.qml \
    ../../interface/resources/qml/styles/Border.qml \
    ../../interface/resources/qml/styles/HifiConstants.qml \
    ../../interface/resources/qml/windows/DefaultFrame.qml \
    ../../interface/resources/qml/windows/Fadable.qml \
    ../../interface/resources/qml/windows/Frame.qml \
    ../../interface/resources/qml/windows/ModalFrame.qml \
    ../../interface/resources/qml/windows/Window.qml \
    ../../interface/resources/qml/AddressBarDialog.qml \
    ../../interface/resources/qml/AvatarInputs.qml \
    ../../interface/resources/qml/Browser.qml \
    ../../interface/resources/qml/InfoView.qml \
    ../../interface/resources/qml/LoginDialog.qml \
    ../../interface/resources/qml/QmlWebWindow.qml \
    ../../interface/resources/qml/QmlWindow.qml \
    ../../interface/resources/qml/Root.qml \
    ../../interface/resources/qml/ScrollingGraph.qml \
    ../../interface/resources/qml/Stats.qml \
    ../../interface/resources/qml/TextOverlayElement.qml \
    ../../interface/resources/qml/Tooltip.qml \
    ../../interface/resources/qml/ToolWindow.qml \
    ../../interface/resources/qml/UpdateDialog.qml \
    ../../interface/resources/qml/VrMenu.qml \
    ../../interface/resources/qml/VrMenuItem.qml \
    ../../interface/resources/qml/VrMenuView.qml \
    ../../interface/resources/qml/WebEntity.qml \
    ../../interface/resources/qml/desktop/Desktop.qml \
    ../../interface/resources/qml/hifi/Desktop.qml \
    ../../interface/resources/qml/menus/MenuMouseHandler.qml \
    ../../interface/resources/qml/menus/VrMenuItem.qml \
    ../../interface/resources/qml/menus/VrMenuView.qml \
    ../../interface/resources/qml/windows/ModalWindow.qml \
    ../../interface/resources/qml/desktop/FocusHack.qml \
    ../../interface/resources/qml/hifi/dialogs/PreferencesDialog.qml \
    ../../interface/resources/qml/hifi/dialogs/Section.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/Browsable.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/Section.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/Editable.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/Slider.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/Preference.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/CheckBox.qml \
    ../../interface/resources/qml/dialogs/fileDialog/FileTableView.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/Avatar.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/AvatarBrowser.qml \
    ../../interface/resources/qml/dialogs/QueryDialog.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/Button.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/AvatarPreference.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/BrowsablePreference.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/ButtonPreference.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/CheckBoxPreference.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/EditablePreference.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/SliderPreference.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/SpinBoxPreference.qml

HEADERS += \
    ../../libraries/ui/src/FileDialogHelper.h
