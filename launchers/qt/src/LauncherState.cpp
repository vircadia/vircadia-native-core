#include "LauncherState.h"

#include <array>

#include <QQmlEngine>

static const std::array<QString, LauncherState::UIState::UI_STATE_NUM> QML_FILE_FOR_UI_STATE =
    { { "qrc:/qml/SplashScreen.qml", "qrc:/qml/Login.qml", "qrc:/qml/DisplayName.qml",
        "qrc:/qml/Download.qml", "qrc:/qml/DownloadFinshed.qml", "qrc:/qml/Error.qml" } };

QString LauncherState::getCurrentUISource() const {
    return QML_FILE_FOR_UI_STATE[getUIState()];
}

void LauncherState::declareQML() {
    qmlRegisterType<LauncherState>("HQLauncher", 1, 0, "LauncherStateEnums");
}

void LauncherState::setUIState(UIState state) {
    _uiState = state;
}

LauncherState::UIState LauncherState::getUIState() const {
    return _uiState;
}

void LauncherState::setLastLoginError(LastLoginError lastLoginError) {
    _lastLoginError = lastLoginError;
}

LauncherState::LastLoginError LauncherState::getLastLoginError() const {
    return _lastLoginError;
}
