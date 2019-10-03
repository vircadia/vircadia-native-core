#include "LauncherState.h"

#include "CommandlineOptions.h"
#include "PathUtils.h"
#include "Unzipper.h"
#include "Helper.h"

#include <array>
#include <cstdlib>

#include <QProcess>

#include <QNetworkRequest>
#include <QNetworkReply>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

#include <QDebug>
#include <QQmlEngine>

#include <QThreadPool>

#include <QStandardPaths>
#include <QEventLoop>

#include <qregularexpression.h>

//#define BREAK_ON_ERROR

const QString configHomeLocationKey { "homeLocation" };
const QString configLoggedInKey{ "loggedIn" };
const QString configLauncherPathKey{ "launcherPath" };

QString LauncherState::getContentCachePath() const {
    return _launcherDirectory.filePath("cache");
}

QString LauncherState::getClientDirectory() const {
    return _launcherDirectory.filePath("interface_install");
}

QString LauncherState::getClientExecutablePath() const {
    QDir clientDirectory = getClientDirectory();
#if defined(Q_OS_WIN)
    return clientDirectory.absoluteFilePath("interface.exe");
#elif defined(Q_OS_MACOS)
    return clientDirectory.absoluteFilePath("interface.app/Contents/MacOS/interface");
#endif
}


QString LauncherState::getConfigFilePath() const {
    QDir clientDirectory = getClientDirectory();
    return clientDirectory.absoluteFilePath("config.json");
}

QString LauncherState::getLauncherFilePath() const {
#if defined(Q_OS_WIN)
    return _launcherDirectory.absoluteFilePath("launcher.exe");
#elif defined(Q_OS_MACOS)
    return _launcherDirectory.absoluteFilePath("launcher.app");
#endif
}

bool LauncherState::shouldDownloadContentCache() const {
    return !_contentCacheURL.isEmpty() && !QFile::exists(getContentCachePath());
}

static const std::array<QString, LauncherState::UIState::UI_STATE_NUM> QML_FILE_FOR_UI_STATE =
    { { "qml/SplashScreen.qml", "qml/HFBase/CreateAccountBase.qml", "qml/HFBase/LoginBase.qml", "DisplayName.qml",
        "qml/Download.qml", "qml/DownloadFinished.qml", "qml/HFBase/Error.qml" } };

void LauncherState::ASSERT_STATE(LauncherState::ApplicationState state) {
    if (_applicationState != state) {
#ifdef BREAK_ON_ERROR
        __debugbreak();
#endif
        setApplicationState(ApplicationState::UnexpectedError);
    }
}

void LauncherState::ASSERT_STATE(const std::vector<LauncherState::ApplicationState>& states) {
    for (auto state : states) {
        if (_applicationState == state) {
            return;
        }
    }

#ifdef BREAK_ON_ERROR
        __debugbreak();
#endif
    setApplicationState(ApplicationState::UnexpectedError);
}

LauncherState::LauncherState() {
    _launcherDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // TODO Fix launcher directory
    qDebug() << "Launcher directory: " << _launcherDirectory.absolutePath();
    _launcherDirectory.mkpath(_launcherDirectory.absolutePath());
    requestBuilds();
}

QString LauncherState::getCurrentUISource() const {
    return QML_FILE_FOR_UI_STATE[getUIState()];
}

void LauncherState::declareQML() {
    qmlRegisterType<LauncherState>("HQLauncher", 1, 0, "LauncherStateEnums");
}

LauncherState::UIState LauncherState::getUIState() const {
    switch (_applicationState) {
        case ApplicationState::Init:
        case ApplicationState::RequestingBuilds:
        case ApplicationState::GettingCurrentClientVersion:
            return SPLASH_SCREEN;
        case ApplicationState::WaitingForLogin:
        case ApplicationState::RequestingLogin:
            return LOGIN_SCREEN;
        case ApplicationState::WaitingForSignup:
        case ApplicationState::RequestingSignup:
            return SIGNUP_SCREEN;
        case ApplicationState::DownloadingClient:
        case ApplicationState::InstallingClient:
        case ApplicationState::DownloadingContentCache:
        case ApplicationState::InstallingContentCache:
            return DOWNLOAD_SCREEN;
        case ApplicationState::LaunchingHighFidelity:
            return DOWNLOAD_FINSISHED;
        case ApplicationState::UnexpectedError:
#ifdef BREAK_ON_ERROR
            __debugbreak();
#endif
            return ERROR_SCREEN;
        default:
            qDebug() << "FATAL: No UI for" << _applicationState;
#ifdef BREAK_ON_ERROR
            __debugbreak();
#endif
            return ERROR_SCREEN;
    }
}

void LauncherState::setLastLoginError(LastLoginError lastLoginError) {
    _lastLoginError = lastLoginError;
}

LauncherState::LastLoginError LauncherState::getLastLoginError() const {
    return _lastLoginError;
}

void LauncherState::restart() {
    setApplicationState(ApplicationState::Init);
    requestBuilds();
}

void LauncherState::requestBuilds() {
    ASSERT_STATE(ApplicationState::Init);
    setApplicationState(ApplicationState::RequestingBuilds);

    auto request = new BuildsRequest();

    QObject::connect(request, &BuildsRequest::finished, this, [=] {
        ASSERT_STATE(ApplicationState::RequestingBuilds);
        if (request->getError() != BuildsRequest::Error::None) {
            setApplicationStateError("Could not retrieve latest builds");
            return;
        }

        _latestBuilds = request->getLatestBuilds();

        CommandlineOptions* options = CommandlineOptions::getInstance();
        if (shouldDownloadLauncher() && !options->contains("--noUpdate")) {
            downloadLauncher();
            return;
        }
        getCurrentClientVersion();
    });

    request->send(_networkAccessManager);
}

bool LauncherState::shouldDownloadLauncher() {
    return _latestBuilds.launcherBuild.latestVersion != atoi(LAUNCHER_BUILD_VERSION);
}

void LauncherState::getCurrentClientVersion() {
    ASSERT_STATE(ApplicationState::RequestingBuilds);

    setApplicationState(ApplicationState::GettingCurrentClientVersion);

    QProcess client;
    QEventLoop loop;

    connect(&client, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::exit, Qt::QueuedConnection);
    connect(&client, &QProcess::errorOccurred, &loop, &QEventLoop::exit, Qt::QueuedConnection);

    client.start(getClientExecutablePath(), { "--version" });
    loop.exec();

    // TODO Handle errors
    auto output = client.readAllStandardOutput();

    QRegularExpression regex { "Interface (?<version>\\d+)(-.*)?" };

    auto match = regex.match(output);

    if (match.hasMatch()) {
        _currentClientVersion = match.captured("version");
    } else {
        _currentClientVersion = QString::null;
    }
    qDebug() << "Current client version is: " << _currentClientVersion;

    {
        auto path = getConfigFilePath();
        QFile configFile{ path };

        if (configFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
            auto root = doc.object();

            _config.launcherPath = getLauncherFilePath();
            _config.loggedIn = false;
            if (root.contains(configLoggedInKey)) {
                _config.loggedIn = root["loggedIn"].toBool();
            }
            if (root.contains(configHomeLocationKey)) {
                _config.homeLocation = root["homeLocation"].toString();
            }
        } else {
            qDebug() << "Failed to open config.json";
        }
    }

    if (_config.loggedIn) {
        downloadClient();
    } else {
        setApplicationState(ApplicationState::WaitingForSignup);
    }
}


void LauncherState::gotoSignup() {
    if (_applicationState == ApplicationState::WaitingForLogin) {
        setApplicationState(ApplicationState::WaitingForSignup);
    } else {
        qDebug() << "Error, can't switch to signup page, current state is: " << _applicationState;
    }
}

void LauncherState::gotoLogin() {
    if (_applicationState == ApplicationState::WaitingForSignup) {
        setApplicationState(ApplicationState::WaitingForLogin);
    } else {
        qDebug() << "Error, can't switch to signup page, current state is: " << _applicationState;
    }
}

void LauncherState::signup(QString email, QString username, QString password, QString displayName) {
    ASSERT_STATE(ApplicationState::WaitingForSignup);

    _username = username;
    _password = password;

    setApplicationState(ApplicationState::RequestingSignup);

    auto signupRequest = new SignupRequest();

    _displayName = displayName;

    {
        _lastSignupError = SignupRequest::Error::None;
        emit lastSignupErrorChanged();
    }

    QObject::connect(signupRequest, &SignupRequest::finished, this, [this, signupRequest] {
        signupRequest->deleteLater();


        _lastSignupError = signupRequest->getError();
        emit lastSignupErrorChanged();

        if (_lastSignupError != SignupRequest::Error::None) {
            setApplicationStateError("Failed to sign up");
            return;
        }

        setApplicationState(ApplicationState::RequestingLoginAfterSignup);

        // After successfully signing up, attempt to login
        auto loginRequest = new LoginRequest();

        connect(loginRequest, &LoginRequest::finished, this, [this, loginRequest]() {
            ASSERT_STATE(ApplicationState::RequestingLoginAfterSignup);

            loginRequest->deleteLater();

            auto err = loginRequest->getError();
            if (err != LoginRequest::Error::None) {
                setApplicationStateError("Failed to login");
                return;
            }


            _config.loggedIn = true;
            _loginResponse = loginRequest->getToken();
            _loginTokenResponse = loginRequest->getRawToken();

            requestSettings();
        });

        setApplicationState(ApplicationState::RequestingLoginAfterSignup);
        loginRequest->send(_networkAccessManager, _username, _password);
    });
    signupRequest->send(_networkAccessManager, email, username, password);
}


void LauncherState::login(QString username, QString password, QString displayName) {
    ASSERT_STATE(ApplicationState::WaitingForLogin);

    setApplicationState(ApplicationState::RequestingLogin);

    _displayName = displayName;

    qDebug() << "Got login: " << username << password;

    auto request = new LoginRequest();

    connect(request, &LoginRequest::finished, this, [this, request]() {
        ASSERT_STATE(ApplicationState::RequestingLogin);

        request->deleteLater();

        auto err = request->getError();
        if (err != LoginRequest::Error::None) {
            setApplicationStateError("Failed to login");
            return;
        }

        _config.loggedIn = true;
        _loginResponse = request->getToken();
        _loginTokenResponse = request->getRawToken();

        requestSettings();
    });

    request->send(_networkAccessManager, username, password);
}

void LauncherState::requestSettings() {
    // TODO Request settings if already logged in
    qDebug() << "Requesting settings";

    auto request = new UserSettingsRequest();

    connect(request, &UserSettingsRequest::finished, this, [this, request]() {
        auto userSettings = request->getUserSettings();
        if (userSettings.homeLocation.isEmpty()) {
            _config.homeLocation = "hifi://hq";
            _contentCacheURL = "";
        } else {
            _config.homeLocation = userSettings.homeLocation;
            auto host = QUrl(_config.homeLocation).host();
            _contentCacheURL = "http://orgs.highfidelity.com/host-content-cache/" +  host + ".zip";

            qDebug() << "Content cache url: " << _contentCacheURL;
        }

        qDebug() << "Home location is: " << _config.homeLocation;
        qDebug() << "Content cache url is: " << _contentCacheURL;

        downloadClient();
    });

    request->send(_networkAccessManager, _loginResponse);
}

void LauncherState::downloadClient() {
    ASSERT_STATE(ApplicationState::RequestingLogin);

    Build build;
    if (!_latestBuilds.getBuild(_buildTag, &build)) {
        qDebug() << "Cannot determine latest build";
        setApplicationState(ApplicationState::UnexpectedError);
        return;
    }

    if (QString::number(build.latestVersion) == _currentClientVersion) {
        qDebug() << "Existing client install is up-to-date.";
        downloadContentCache();
        return;
    }

    _interfaceDownloadProgress = 0;
    setApplicationState(ApplicationState::DownloadingClient);

    // Start client download
    {
        qDebug() << "Latest build: " << build.tag << build.buildNumber << build.latestVersion << build.installerZipURL;
        auto request = new QNetworkRequest(QUrl(build.installerZipURL));
        auto reply = _networkAccessManager.get(*request);

        _clientZipFile.setFileName(_launcherDirectory.absoluteFilePath("client.zip"));

        qDebug() << "Opening " << _clientZipFile.fileName();
        if (!_clientZipFile.open(QIODevice::WriteOnly)) {
            setApplicationState(ApplicationState::UnexpectedError);
            return;
        }

        connect(reply, &QNetworkReply::finished, this, &LauncherState::clientDownloadComplete);
        connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
            char buf[4096];
            while (reply->bytesAvailable() > 0) {
                qint64 size;
                size = reply->read(buf, (qint64)sizeof(buf));
                if (size == 0) {
                    break;
                }
                _clientZipFile.write(buf, size);
            }
        });
        connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
            _interfaceDownloadProgress = (float)received / (float)total;
            emit downloadProgressChanged();
        });
    }
}

void LauncherState::launcherDownloadComplete() {
    _launcherZipFile.close();

#ifdef Q_OS_MAC
    installLauncher();
#elif defined(Q_OS_WIN)
    launchAutoUpdater(_launcherZipFile.fileName());
#endif
}

void LauncherState::clientDownloadComplete() {
    ASSERT_STATE(ApplicationState::DownloadingClient);
    _clientZipFile.close();
    installClient();
}


float LauncherState::calculateDownloadProgress() const{
    if (shouldDownloadContentCache()) {
        return (_interfaceDownloadProgress * 0.40f) + (_interfaceInstallProgress * 0.10f) +
            (_contentInstallProgress * 0.40f) + (_contentDownloadProgress * 0.10f);
    }

    return (_interfaceDownloadProgress * 0.80f) + (_interfaceInstallProgress * 0.20f);
}

void LauncherState::installClient() {
    ASSERT_STATE(ApplicationState::DownloadingClient);
    setApplicationState(ApplicationState::InstallingClient);

    _launcherDirectory.rmpath("interface_install");
    _launcherDirectory.mkpath("interface_install");
    auto installDir = _launcherDirectory.absoluteFilePath("interface_install");

    _interfaceInstallProgress = 0;

    qDebug() << "Unzipping " << _clientZipFile.fileName() << " to " << installDir;

    auto unzipper = new Unzipper(_clientZipFile.fileName(), QDir(installDir));
    unzipper->setAutoDelete(true);
    connect(unzipper, &Unzipper::progress, this, [this](float progress) {
        //qDebug() << "Unzipper progress: " << progress;
        _interfaceInstallProgress = progress;
        emit downloadProgressChanged();
    });
    connect(unzipper, &Unzipper::finished, this, [this](bool error, QString errorMessage) {
        if (error) {
            qDebug() << "Unzipper finished with error: " << errorMessage;
            setApplicationState(ApplicationState::UnexpectedError);
        } else {
            qDebug() << "Unzipper finished without error";
            downloadContentCache();
        }
    });
    QThreadPool::globalInstance()->start(unzipper);

    //launchClient();
}

void LauncherState::downloadLauncher() {
    auto request = new QNetworkRequest(QUrl(_latestBuilds.launcherBuild.installerZipURL));
    auto reply = _networkAccessManager.get(*request);

#ifdef Q_OS_MAC
    _launcherZipFile.setFileName(_launcherDirectory.absoluteFilePath("launcher.zip"));
#elif defined(Q_OS_WIN)
    _launcherZipFile.setFileName(_launcherDirectory.absoluteFilePath("launcher.exe"));
#endif

    qDebug() << "opening " << _launcherZipFile.fileName();

    if (!_launcherZipFile.open(QIODevice::WriteOnly)) {
        setApplicationState(ApplicationState::UnexpectedError);
        return;
    }

    connect(reply, &QNetworkReply::finished, this, &LauncherState::launcherDownloadComplete);
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        char buf[4096];
        while (reply->bytesAvailable() > 0) {
            qint64 size;
            size = reply->read(buf, (qint64)sizeof(buf));
            if (size == 0) {
                break;
            }
            _launcherZipFile.write(buf, size);
        }
    });
}

void LauncherState::installLauncher() {
    _launcherDirectory.rmpath("launcher_install");
    _launcherDirectory.mkpath("launcher_install");
     auto installDir = _launcherDirectory.absoluteFilePath("launcher_install");

    qDebug() << "Uzipping " << _launcherZipFile.fileName() << " to " << installDir;

    auto unzipper = new Unzipper(_launcherZipFile.fileName(), QDir(installDir));
    unzipper->setAutoDelete(true);
    connect(unzipper, &Unzipper::finished, this, [this](bool error, QString errorMessage) {
        if (error) {
            qDebug() << "Unzipper finished with error: " << errorMessage;
        } else {
            qDebug() << "Unzipper finished without error";

            QDir installDirectory = _launcherDirectory.filePath("launcher_install");
            QString launcherPath;
#if defined(Q_OS_WIN)
            launcherPath = installDirectory.absoluteFilePath("HQ Launcher.exe");
#elif defined(Q_OS_MACOS)
            launcherPath = installDirectory.absoluteFilePath("HQ Launcher.app");
#endif
            ::launchAutoUpdater(launcherPath);
        }
    });

    QThreadPool::globalInstance()->start(unzipper);
}

void LauncherState::downloadContentCache() {
    ASSERT_STATE({ ApplicationState::RequestingLogin, ApplicationState::InstallingClient });

    // Start content set cache download
    if (shouldDownloadContentCache()) {
        setApplicationState(ApplicationState::DownloadingContentCache);

        _contentDownloadProgress = 0;

        qDebug() << "Downloading content cache from: " << _contentCacheURL;
        QNetworkRequest request{ QUrl(_contentCacheURL) };
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        auto reply = _networkAccessManager.get(request);

        _contentZipFile.setFileName(_launcherDirectory.absoluteFilePath("content_cache.zip"));

        qDebug() << "Opening " << _contentZipFile.fileName();
        if (!_contentZipFile.open(QIODevice::WriteOnly)) {
            setApplicationState(ApplicationState::UnexpectedError);
            return;
        }

        connect(reply, &QNetworkReply::finished, this, &LauncherState::contentCacheDownloadComplete);
        connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
            _contentDownloadProgress = (float)received / (float)total;
            emit downloadProgressChanged();
        });
    } else {
        launchClient();
    }
}

void LauncherState::contentCacheDownloadComplete() {
    ASSERT_STATE(ApplicationState::DownloadingContentCache);

    auto reply = static_cast<QNetworkReply*>(sender());

    if (reply->error()) {
        qDebug() << "Error downloading content cache: " << reply->error() << reply->readAll();
        qDebug() << "Continuing to launch client";
        launchClient();
        return;
    }

    char buf[4096];
    while (reply->bytesAvailable() > 0) {
        qint64 size;
        size = reply->read(buf, (qint64)sizeof(buf));
        _contentZipFile.write(buf, size);
    }

    _contentZipFile.close();

    installContentCache();
}


void LauncherState::installContentCache() {
    ASSERT_STATE(ApplicationState::DownloadingContentCache);
    setApplicationState(ApplicationState::InstallingContentCache);

    auto installDir = getContentCachePath();

    qDebug() << "Unzipping " << _contentZipFile.fileName() << " to " << installDir;

    _contentInstallProgress = 0;

    auto unzipper = new Unzipper(_contentZipFile.fileName(), QDir(installDir));
    unzipper->setAutoDelete(true);
    connect(unzipper, &Unzipper::progress, this, [this](float progress) {
        qDebug() << "Unzipper progress (content cache): " << progress;
        _contentInstallProgress = progress;
        emit downloadProgressChanged();
    });
    connect(unzipper, &Unzipper::finished, this, [this](bool error, QString errorMessage) {
        if (error) {
            qDebug() << "Unzipper finished with error: " << errorMessage;
            setApplicationState(ApplicationState::UnexpectedError);
        } else {
            qDebug() << "Unzipper finished without error";
            launchClient();
        }
    });
    QThreadPool::globalInstance()->start(unzipper);

}

void LauncherState::launchClient() {
    ASSERT_STATE({
        ApplicationState::RequestingLogin,
        ApplicationState::InstallingClient,
        ApplicationState::InstallingContentCache
    });

    setApplicationState(ApplicationState::LaunchingHighFidelity);

    QDir installDirectory = _launcherDirectory.filePath("interface_install");
    QString clientPath;
#if defined(Q_OS_WIN)
    clientPath = installDirectory.absoluteFilePath("interface.exe");
#elif defined(Q_OS_MACOS)
    clientPath = installDirectory.absoluteFilePath("interface.app/Contents/MacOS/interface");
#endif

    auto path = getConfigFilePath();
    QFile configFile{ path };
    if (configFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        doc.setObject({
            { configHomeLocationKey, _config.homeLocation },
            { configLoggedInKey, _config.loggedIn },
            { configLauncherPathKey, _config.launcherPath },
        });
        configFile.write(doc.toJson());
    }

    QString defaultScriptsPath;
#if defined(Q_OS_WIN)
    defaultScriptsPath = installDirectory.filePath("scripts/simplifiedUIBootstrapper.js");
#elif defined(Q_OS_MACOS)
    defaultScriptsPath = installDirectory.filePath("interface.app/Contents/Resources/scripts/simplifiedUIBootstrapper.js");
#endif

    QString contentCachePath = _launcherDirectory.filePath("cache");

    ::launchClient(clientPath, _config.homeLocation, defaultScriptsPath, _displayName, contentCachePath, _loginTokenResponse);
}

void LauncherState::setApplicationStateError(QString errorMessage) {
    _applicationErrorMessage = errorMessage;
    setApplicationState(ApplicationState::UnexpectedError);
}

void LauncherState::setApplicationState(ApplicationState state) {
    qDebug() << "Changing application state: " << _applicationState << " -> " << state;

    if (state == ApplicationState::UnexpectedError) {
#ifdef BREAK_ON_ERROR
        __debugbreak();
#endif
    }

    _applicationState = state;
    UIState updatedUIState = getUIState();
    if (_uiState != updatedUIState) {
        emit uiStateChanged();
        emit updateSourceUrl(PathUtils::resourcePath(getCurrentUISource()));
        _uiState = getUIState();
    }

    emit applicationStateChanged();
}

LauncherState::ApplicationState LauncherState::getApplicationState() const {
    return _applicationState;
}
