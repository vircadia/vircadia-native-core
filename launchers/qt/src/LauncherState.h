#pragma once

#include <QDir>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QFile>

#include "LoginRequest.h"
#include "SignupRequest.h"
#include "UserSettingsRequest.h"
#include "BuildsRequest.h"

struct LauncherConfig {
    QString launcherPath{ "" };
    bool loggedIn{ false };
    QString homeLocation{ "" };
};

class LauncherState : public QObject {
    Q_OBJECT

    Q_PROPERTY(UIState uiState READ getUIState NOTIFY uiStateChanged)
    Q_PROPERTY(ApplicationState applicationState READ getApplicationState NOTIFY applicationStateChanged)
    Q_PROPERTY(float downloadProgress READ getDownloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(SignupRequest::Error lastSignupError MEMBER _lastSignupError NOTIFY lastSignupErrorChanged)
    Q_PROPERTY(QString buildVersion READ getBuildVersion)

public:
    LauncherState();
    ~LauncherState() = default;

    enum UIState {
        SPLASH_SCREEN = 0,
        SIGNUP_SCREEN,
        LOGIN_SCREEN,
        DISPLAY_NAME_SCREEN,
        DOWNLOAD_SCREEN,
        DOWNLOAD_FINSISHED,
        ERROR_SCREEN,
        UI_STATE_NUM
    };

    enum class ApplicationState {
        Init,

        UnexpectedError,

        RequestingBuilds,
        GettingCurrentClientVersion,

        WaitingForLogin,
        RequestingLogin,

        WaitingForSignup,
        RequestingSignup,
        RequestingLoginAfterSignup,

        DownloadingClient,
        DownloadingLauncher,
        DownloadingContentCache,

        InstallingClient,
        InstallingLauncher,
        InstallingContentCache,

        LaunchingHighFidelity
    };

    enum LastLoginError {
        NONE = 0,
        ORGINIZATION,
        CREDENTIALS,
        LAST_ERROR_NUM
    };

    Q_ENUM(UIState);
    Q_ENUM(ApplicationState)
    Q_ENUM(LastLoginError)

    Q_INVOKABLE QString getCurrentUISource() const;

    void ASSERT_STATE(LauncherState::ApplicationState state);
    void ASSERT_STATE(const std::vector<LauncherState::ApplicationState>& states);

    static void declareQML();

    UIState getUIState() const;

    QString getBuildVersion() { return QString(LAUNCHER_BUILD_VERSION); }

    void setLastLoginError(LastLoginError lastLoginError);
    LastLoginError getLastLoginError() const;

    void setApplicationStateError(QString errorMessage);
    void setApplicationState(ApplicationState state);
    ApplicationState getApplicationState() const;

    Q_INVOKABLE void gotoSignup();
    Q_INVOKABLE void gotoLogin();

    // Request builds
    void requestBuilds();

    // Signup
    Q_INVOKABLE void signup(QString email, QString username, QString password, QString displayName);

    // Login
    Q_INVOKABLE void login(QString username, QString password, QString displayName);

    // Request Settings
    void requestSettings();

    Q_INVOKABLE void restart();

    // Launcher
    void downloadLauncher();
    void installLauncher();

    // Client
    void downloadClient();
    void installClient();

    // Content Cache
    void downloadContentCache();
    void installContentCache();

    // Launching
    void launchClient();

    Q_INVOKABLE float getDownloadProgress() const { return calculateDownloadProgress(); }

signals:
    void updateSourceUrl(QUrl sourceUrl);
    void uiStateChanged();
    void applicationStateChanged();
    void downloadProgressChanged();
    void lastSignupErrorChanged();

private slots:
    void clientDownloadComplete();
    void contentCacheDownloadComplete();
    void launcherDownloadComplete();

private:
    bool shouldDownloadContentCache() const;
    void getCurrentClientVersion();

    QString getContentCachePath() const;
    QString getClientDirectory() const;
    QString getClientExecutablePath() const;
    QString getConfigFilePath() const;
    QString getLauncherFilePath() const;

    float calculateDownloadProgress() const;

    bool shouldDownloadLauncher();

    QNetworkAccessManager _networkAccessManager;
    Builds _latestBuilds;
    QDir _launcherDirectory;

    LauncherConfig _config;

    // Application State
    ApplicationState _applicationState { ApplicationState::Init };
    UIState _uiState { UIState::SPLASH_SCREEN };
    LoginToken _loginResponse;
    LastLoginError _lastLoginError { NONE };
    SignupRequest::Error _lastSignupError{ SignupRequest::Error::None };
    QString _displayName;
    QString _applicationErrorMessage;
    QString _currentClientVersion;
    QString _buildTag { QString::null };
    QString _contentCacheURL;
    QString _loginTokenResponse;
    QFile _clientZipFile;
    QFile _launcherZipFile;
    QFile _contentZipFile;

    QString _username;
    QString _password;

    float _downloadProgress { 0 };
    float _contentDownloadProgress { 0 };
    float _contentInstallProgress { 0 };
    float _interfaceDownloadProgress { 0 };
    float _interfaceInstallProgress { 0 };
};
