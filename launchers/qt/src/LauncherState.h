#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

struct Build {
    QString tag;
    int latestVersion;
    int buildNumber;
    QString installerZipURL;
};

struct LatestBuilds {
    QString defaultTag;
    std::vector<Build> builds;
};

class LauncherState : public QObject {
    Q_OBJECT

public:
    LauncherState();
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

    enum class ApplicationState {
        INIT,

        REQUESTING_BUILDS,
        REQUESTING_BUILDS_FAILED,

        WAITING_FOR_LOGIN,
        REQUESTING_LOGIN,

        WAITING_FOR_SIGNUP,
        REQUESTING_SIGNUP,

        DOWNLOADING_CONTENT,
        DOWNLOADING_HIGH_FIDELITY,

        EXTRACTING_DATA,

        LAUNCHING_HIGH_FIDELITY
    };
    Q_ENUMS(ApplicationState);


    enum LastLoginError {
        NONE = 0,
        ORGINIZATION,
        CREDENTIALS,
        LAST_ERROR_NUM
    };
    Q_ENUMS(LastLoginError);
    Q_INVOKABLE QString getCurrentUISource() const;

    void LauncherState::ASSERT_STATE(LauncherState::ApplicationState state) const;

    static void declareQML();

    void setUIState(UIState state);
    UIState getUIState() const;

    void setLastLoginError(LastLoginError lastLoginError);
    LastLoginError getLastLoginError() const;

    // Request builds
    void requestBuilds();
    Q_INVOKABLE void receivedBuildsReply();

    // Login
    Q_INVOKABLE void login(QString username, QString password);
    Q_INVOKABLE void receivedLoginReply();

    // Download
    void download();
    Q_INVOKABLE void contentDownloadComplete();
    Q_INVOKABLE void clientDownloadComplete();

    // Launching
    void launchClient();

signals:
    void updateSourceUrl(QString sourceUrl);

private:
    QNetworkAccessManager _networkAccessManager;
    LatestBuilds _latestBuilds;

    ApplicationState _appState { ApplicationState::INIT };
    UIState _uiState { SPLASH_SCREEN };
    LastLoginError _lastLoginError { NONE };
};
