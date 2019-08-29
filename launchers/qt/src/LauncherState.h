#include <QObject>
#include <QString>

class LauncherState : public QObject {
    Q_OBJECT

public:
    LauncherState() = default;
    ~LauncherState() = default;

    enum UIState {
        SPLASH_SCREEN = 0,
        LOGIN_SCREEN,
        DISPLAY_NAME_SCREEN,
        DOWNLOAD_SCREEN,
        DOWNLOAD_FINSISHED,
        ERROR_SCREEN,
        UI_STATE_NUM
    };
    Q_ENUMS(UIState);

    enum LastLoginError {
        NONE = 0,
        ORGINIZATION,
        CREDENTIALS,
        LAST_ERROR_NUM
    };
    Q_ENUMS(LastLoginError);
    Q_INVOKABLE QString getCurrentUISource() const;

    static void declareQML();

    void setUIState(UIState state);
    UIState getUIState() const;

    void setLastLoginError(LastLoginError lastLoginError);
    LastLoginError getLastLoginError() const;

signals:
    void updateSourceUrl(QString sourceUrl);

private:
    UIState _uiState { SPLASH_SCREEN };
    LastLoginError _lastLoginError { NONE };
};
