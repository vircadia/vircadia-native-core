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
    QString lastLogin{ "" };
    QString launcherPath{ "" };
    bool loggedIn{ false };
    QString homeLocation{ "" };
};

class LauncherState : public QObject {
    Q_OBJECT

    Q_PROPERTY(UIState uiState READ getUIState NOTIFY uiStateChanged)
    Q_PROPERTY(ApplicationState applicationState READ getApplicationState NOTIFY applicationStateChanged)
    Q_PROPERTY(float downloadProgress READ getDownloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString lastLoginErrorMessage READ getLastLoginErrorMessage NOTIFY lastLoginErrorMessageChanged)
    Q_PROPERTY(QString lastSignupErrorMessage READ getLastSignupErrorMessage NOTIFY lastSignupErrorMessageChanged)
    Q_PROPERTY(QString buildVersion READ getBuildVersion)
    Q_PROPERTY(QString lastUsedUsername READ getLastUsedUsername)

public:
    LauncherState();
    ~LauncherState() = default;

    enum UIState {
        SplashScreen = 0,
        SignupScreen,
        LoginScreen,
        DownloadScreen,
        DownloadFinishedScreen,
        ErrorScreen,

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

        LaunchingHighFidelity,
    };

    Q_ENUM(ApplicationState)

    bool _isDebuggingScreens{ false };
    UIState _currentDebugScreen{ UIState::SplashScreen };
    void toggleDebugState();
    void gotoNextDebugScreen();
    void gotoPreviousDebugScreen();

    Q_INVOKABLE QString getCurrentUISource() const;

    void ASSERT_STATE(ApplicationState state);
    void ASSERT_STATE(const std::vector<ApplicationState>& states);

    static void declareQML();

    UIState getUIState() const;

    void setLastLoginErrorMessage(const QString& msg);
    QString getLastLoginErrorMessage() const { return _lastLoginErrorMessage; }

    void setLastSignupErrorMessage(const QString& msg);
    QString getLastSignupErrorMessage() const { return _lastSignupErrorMessage; }

    QString getBuildVersion();
    QString getLastUsedUsername() const { return _lastUsedUsername; }

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

    Q_INVOKABLE void openURLInBrowser(QString url);

signals:
    void updateSourceUrl(QUrl sourceUrl);
    void uiStateChanged();
    void applicationStateChanged();
    void downloadProgressChanged();
    void lastSignupErrorChanged();
    void lastSignupErrorMessageChanged();
    void lastLoginErrorMessageChanged();

private slots:
    void clientDownloadComplete();
    void contentCacheDownloadComplete();
    void launcherDownloadComplete();

private:
    bool shouldDownloadContentCache() const;
    void getCurrentClientVersion();

    float calculateDownloadProgress() const;

    bool shouldDownloadLauncher();

    QNetworkAccessManager _networkAccessManager;
    Builds _latestBuilds;
    QDir _launcherDirectory;

    LauncherConfig _config;

    // Application State
    ApplicationState _applicationState { ApplicationState::Init };
    UIState _uiState { UIState::SplashScreen };
    LoginToken _loginResponse;
    SignupRequest::Error _lastSignupError{ SignupRequest::Error::None };
    QString _lastLoginErrorMessage{ "" };
    QString _lastSignupErrorMessage{ "" };
    QString _lastUsedUsername;
    QString _displayName;
    QString _applicationErrorMessage;
    QString _currentClientVersion;
    QString _buildTag;
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
