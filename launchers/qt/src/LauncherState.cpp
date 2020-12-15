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

#include <QEventLoop>

#include <qregularexpression.h>

#ifdef Q_OS_WIN
#include <shellapi.h>
#endif

//#define BREAK_ON_ERROR
//#define DEBUG_UI

const QString configHomeLocationKey { "homeLocation" };
const QString configLastLoginKey { "lastLogin" };
const QString configLoggedInKey{ "loggedIn" };
const QString configLauncherPathKey{ "launcherPath" };

Q_INVOKABLE void LauncherState::openURLInBrowser(QString url) {
#ifdef Q_OS_WIN
    ShellExecute(0, 0, url.toLatin1(), 0, 0 , SW_SHOW);
#elif defined(Q_OS_DARWIN)
    system("open \"" + url.toLatin1() + "\"");
#endif
}

void LauncherState::toggleDebugState() {
#ifdef DEBUG_UI
    _isDebuggingScreens = !_isDebuggingScreens;

    UIState updatedUIState = getUIState();
    if (_uiState != updatedUIState) {
        emit uiStateChanged();
        emit updateSourceUrl(PathUtils::resourcePath(getCurrentUISource()));
        _uiState = getUIState();
    }
#endif
}
void LauncherState::gotoNextDebugScreen() {
#ifdef DEBUG_UI
    if (_currentDebugScreen < (UIState::UI_STATE_NUM - 1)) {
        _currentDebugScreen = (UIState)(_currentDebugScreen + 1);
        emit uiStateChanged();
        emit updateSourceUrl(PathUtils::resourcePath(getCurrentUISource()));
        _uiState = getUIState();
    }
#endif
}
void LauncherState::gotoPreviousDebugScreen() {
#ifdef DEBUG_UI
    if (_currentDebugScreen > 0) {
        _currentDebugScreen = (UIState)(_currentDebugScreen - 1);
        emit uiStateChanged();
        emit updateSourceUrl(PathUtils::resourcePath(getCurrentUISource()));
        _uiState = getUIState();
    }
#endif
}

bool LauncherState::shouldDownloadContentCache() const {
    return !_contentCacheURL.isEmpty() && !QFile::exists(PathUtils::getContentCachePath());
}

void LauncherState::setLastSignupErrorMessage(const QString& msg) {
    _lastSignupErrorMessage = msg;
    emit lastSignupErrorMessageChanged();
}

void LauncherState::setLastLoginErrorMessage(const QString& msg) {
    _lastLoginErrorMessage = msg;
    emit lastLoginErrorMessageChanged();
}

static const std::array<QString, LauncherState::UIState::UI_STATE_NUM> QML_FILE_FOR_UI_STATE =
    { { "qml/SplashScreen.qml", "qml/HFBase/CreateAccountBase.qml", "qml/HFBase/LoginBase.qml",
        "qml/Download.qml", "qml/DownloadFinished.qml", "qml/HFBase/Error.qml" } };

void LauncherState::ASSERT_STATE(ApplicationState state) {
    if (_applicationState != state) {
        qDebug() << "Unexpected state, current: " << _applicationState << ", expected: " << state;
#ifdef BREAK_ON_ERROR
        __debugbreak();
#endif
    }
}

void LauncherState::ASSERT_STATE(const std::vector<ApplicationState>& states) {
    for (auto state : states) {
        if (_applicationState == state) {
            return;
        }
    }

    qDebug() << "Unexpected state, current: " << _applicationState << ", expected: " << states;
#ifdef BREAK_ON_ERROR
        __debugbreak();
#endif
}

LauncherState::LauncherState() {
    _launcherDirectory = PathUtils::getLauncherDirectory();
    qDebug() << "Launcher directory: " << _launcherDirectory.absolutePath();
    _launcherDirectory.mkpath(_launcherDirectory.absolutePath());
    _launcherDirectory.mkpath(PathUtils::getDownloadDirectory().absolutePath());
    requestBuilds();
}

QString LauncherState::getCurrentUISource() const {
    return QML_FILE_FOR_UI_STATE[getUIState()];
}

void LauncherState::declareQML() {
    qmlRegisterType<LauncherState>("HQLauncher", 1, 0, "ApplicationState");
}

LauncherState::UIState LauncherState::getUIState() const {
    if (_isDebuggingScreens) {
        return _currentDebugScreen;
    }

    switch (_applicationState) {
        case ApplicationState::Init:
        case ApplicationState::RequestingBuilds:
        case ApplicationState::GettingCurrentClientVersion:
            return UIState::SplashScreen;
        case ApplicationState::WaitingForLogin:
        case ApplicationState::RequestingLogin:
            return UIState::LoginScreen;
        case ApplicationState::WaitingForSignup:
        case ApplicationState::RequestingSignup:
        case ApplicationState::RequestingLoginAfterSignup:
            return UIState::SignupScreen;
        case ApplicationState::DownloadingClient:
        case ApplicationState::InstallingClient:
        case ApplicationState::DownloadingContentCache:
        case ApplicationState::InstallingContentCache:
            return UIState::DownloadScreen;
        case ApplicationState::LaunchingHighFidelity:
            return UIState::DownloadFinishedScreen;
        case ApplicationState::UnexpectedError:
#ifdef BREAK_ON_ERROR
            __debugbreak();
#endif
            return UIState::ErrorScreen;
        default:
            qDebug() << "FATAL: No UI for" << _applicationState;
#ifdef BREAK_ON_ERROR
            __debugbreak();
#endif
            return UIState::ErrorScreen;
    }
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
        qDebug() << "Latest version: " << _latestBuilds.launcherBuild.latestVersion
                 << "Curretn version: " << getBuildVersion().toInt();
        if (shouldDownloadLauncher() && !options->contains("--noUpdate")) {
            downloadLauncher();
            return;
        }
        getCurrentClientVersion();
    });

    request->send(_networkAccessManager);
}

QString LauncherState::getBuildVersion() {
    QString buildVersion { LAUNCHER_BUILD_VERSION };
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    if (processEnvironment.contains("HQ_LAUNCHER_BUILD_VERSION")) {
        buildVersion = processEnvironment.value("HQ_LAUNCHER_BUILD_VERSION");
    }
    return buildVersion;
}

bool LauncherState::shouldDownloadLauncher() {
    return _latestBuilds.launcherBuild.latestVersion != getBuildVersion().toInt();
}

void LauncherState::getCurrentClientVersion() {
    ASSERT_STATE(ApplicationState::RequestingBuilds);

    setApplicationState(ApplicationState::GettingCurrentClientVersion);

    QProcess client;
    QEventLoop loop;

    connect(&client, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::exit, Qt::QueuedConnection);
    connect(&client, &QProcess::errorOccurred, &loop, &QEventLoop::exit, Qt::QueuedConnection);

    client.start(PathUtils::getClientExecutablePath(), { "--version" });
    loop.exec();

    // TODO Handle errors
    auto output = client.readAllStandardOutput();

    QRegularExpression regex { "Interface (?<version>\\d+)(-.*)?" };

    auto match = regex.match(output);

    if (match.hasMatch()) {
        _currentClientVersion = match.captured("version");
    } else {
        _currentClientVersion.clear();
    }
    qDebug() << "Current client version is: " << _currentClientVersion;

    {
        auto path = PathUtils::getConfigFilePath();
        QFile configFile{ path };

        if (configFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
            auto root = doc.object();

            _config.launcherPath = PathUtils::getLauncherFilePath();
            _config.loggedIn = false;
            if (root.contains(configLoggedInKey)) {
                _config.loggedIn = root[configLoggedInKey].toBool();
            }
            if (root.contains(configLastLoginKey)) {
                _config.lastLogin = root[configLastLoginKey].toString();
            }
            if (root.contains(configHomeLocationKey)) {
                _config.homeLocation = root[configHomeLocationKey].toString();
            }
            if (root.contains(configLauncherPathKey)) {
                _config.launcherPath = root[configLauncherPathKey].toString();
            }
        } else {
            qDebug() << "Failed to open config.json";
        }
    }

    qDebug() << "Is logged-in: " << _config.loggedIn;
    if (_config.loggedIn) {
        downloadClient();
    } else {
        if (_config.lastLogin.isEmpty()) {
            setApplicationState(ApplicationState::WaitingForSignup);
        } else {
            _lastUsedUsername = _config.lastLogin;
            setApplicationState(ApplicationState::WaitingForLogin);
        }
    }
}


void LauncherState::gotoSignup() {
    if (_applicationState == ApplicationState::WaitingForLogin) {
        setLastSignupErrorMessage("");
        _lastLoginErrorMessage = "";
        setApplicationState(ApplicationState::WaitingForSignup);
    } else {
        qDebug() << "Error, can't switch to signup page, current state is: " << _applicationState;
    }
}

void LauncherState::gotoLogin() {
    if (_applicationState == ApplicationState::WaitingForSignup) {
        setLastLoginErrorMessage("");
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

    QObject::connect(signupRequest, &SignupRequest::finished, this, [this, signupRequest, username] {
        signupRequest->deleteLater();


        _lastSignupError = signupRequest->getError();
        emit lastSignupErrorChanged();

        auto err = signupRequest->getError();
        if (err == SignupRequest::Error::ExistingUsername) {
            setLastSignupErrorMessage(_username + " is already taken. Please try a different username.");
            setApplicationState(ApplicationState::WaitingForSignup);
            return;
        } else if (err == SignupRequest::Error::BadPassword) {
            setLastSignupErrorMessage("That's an invalid password. Must be at least 6 characters.");
            setApplicationState(ApplicationState::WaitingForSignup);
            return;
        } else if (err == SignupRequest::Error::BadUsername) {
            setLastSignupErrorMessage("That's an invalid username. Please try another username.");
            setApplicationState(ApplicationState::WaitingForSignup);
            return;
        } else if (err == SignupRequest::Error::UserProfileAlreadyCompleted) {
            setLastSignupErrorMessage("An account with this email already exists. Please <b><a href='login'>log in</a></b>.");
            setApplicationState(ApplicationState::WaitingForSignup);
            return;
        } else if (err == SignupRequest::Error::NoSuchEmail) {
            setLastSignupErrorMessage("That email isn't setup yet. <a href='https://www.highfidelity.com/hq-support'>Request access</a>.");
            setApplicationState(ApplicationState::WaitingForSignup);
            return;
        } else if (err != SignupRequest::Error::None) {
            setApplicationStateError("Failed to sign up. Please try again.");
            return;
        }

        setApplicationState(ApplicationState::RequestingLoginAfterSignup);

        // After successfully signing up, attempt to login
        auto loginRequest = new LoginRequest();

        _lastUsedUsername = username;
        _config.lastLogin = username;

        connect(loginRequest, &LoginRequest::finished, this, [this, loginRequest]() {
            ASSERT_STATE(ApplicationState::RequestingLoginAfterSignup);

            loginRequest->deleteLater();

            auto err = loginRequest->getError();
            if (err == LoginRequest::Error::BadUsernameOrPassword) {
                setLastLoginErrorMessage("Invalid username or password.");
                setApplicationState(ApplicationState::WaitingForLogin);
                return;
            } else if (err != LoginRequest::Error::None) {
                setApplicationStateError("Failed to login. Please try again.");
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

    auto request = new LoginRequest();

    connect(request, &LoginRequest::finished, this, [this, request, username]() {
        ASSERT_STATE(ApplicationState::RequestingLogin);

        request->deleteLater();

        auto err = request->getError();
        if (err == LoginRequest::Error::BadUsernameOrPassword) {
            setLastLoginErrorMessage("Invalid username or password");
            setApplicationState(ApplicationState::WaitingForLogin);
            return;
        } else if (err != LoginRequest::Error::None) {
            setApplicationStateError("Failed to login. Please try again.");
            return;
        }

        _lastUsedUsername = username;
        _config.lastLogin = username;
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
            _config.homeLocation = "file:///~/serverless/tutorial.json";
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
    ASSERT_STATE({ ApplicationState::RequestingLogin, ApplicationState::RequestingLoginAfterSignup });

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

        QDir downloadDir{ PathUtils::getDownloadDirectory() };
        _clientZipFile.setFileName(downloadDir.absoluteFilePath("client.zip"));

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


    auto clientDir = PathUtils::getClientDirectory();

    auto clientPath = clientDir.absolutePath();
    _launcherDirectory.rmpath(clientPath);
    _launcherDirectory.mkpath(clientPath);

    _interfaceInstallProgress = 0;

    qDebug() << "Unzipping " << _clientZipFile.fileName() << " to " << clientDir.absolutePath();

    auto unzipper = new Unzipper(_clientZipFile.fileName(), clientDir);
    unzipper->setAutoDelete(true);
    connect(unzipper, &Unzipper::progress, this, [this](float progress) {
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

        QDir downloadDir{ PathUtils::getDownloadDirectory() };
        _contentZipFile.setFileName(downloadDir.absoluteFilePath("content_cache.zip"));

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
        _contentDownloadProgress = 100.0f;
        _contentInstallProgress = 100.0f;
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

    auto installDir = PathUtils::getContentCachePath();

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

#include <QTimer>
#include <QCoreApplication>
void LauncherState::launchClient() {
    ASSERT_STATE({
        ApplicationState::RequestingLogin,
        ApplicationState::InstallingClient,
        ApplicationState::InstallingContentCache
    });

    setApplicationState(ApplicationState::LaunchingHighFidelity);

    QDir installDirectory = PathUtils::getClientDirectory();
    QString clientPath = PathUtils::getClientExecutablePath();

    auto path = PathUtils::getConfigFilePath();
    QFile configFile{ path };
    if (configFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        doc.setObject({
            { configHomeLocationKey, _config.homeLocation },
            { configLastLoginKey, _config.lastLogin },
            { configLoggedInKey, _config.loggedIn },
            { configLauncherPathKey, PathUtils::getLauncherFilePath() },
        });
        qint64 result = configFile.write(doc.toJson());
        configFile.close();
        qDebug() << "Wrote data to config data: " << result;
    } else {
        qDebug() << "Failed to open config file";
    }

    QString defaultScriptsPath;
#if defined(Q_OS_WIN)
    defaultScriptsPath = installDirectory.filePath("scripts/simplifiedUIBootstrapper.js");
#elif defined(Q_OS_MACOS)
    defaultScriptsPath = installDirectory.filePath("interface.app/Contents/Resources/scripts/simplifiedUIBootstrapper.js");
#endif

    QString contentCachePath = _launcherDirectory.filePath("cache");

    ::launchClient(clientPath, _config.homeLocation, defaultScriptsPath, _displayName, contentCachePath, _loginTokenResponse);
    QTimer::singleShot(3000, QCoreApplication::instance(), &QCoreApplication::quit);
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

