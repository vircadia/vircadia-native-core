#include "LauncherState.h"

#include "PathUtils.h"
#include "Unzipper.h"
#include "Helper.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#include <array>

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


bool LatestBuilds::getBuild(QString tag, Build* outBuild) {
    if (tag.isNull()) {
        tag = defaultTag;
    }

    for (auto& build : builds) {
        if (build.tag == tag) {
            *outBuild = build;
            return true;
        }
    }

    return false;
}

static const std::array<QString, LauncherState::UIState::UI_STATE_NUM> QML_FILE_FOR_UI_STATE =
    { { "SplashScreen.qml", "qml/HFBase/CreateAccountBase.qml", "DisplayName.qml",
        "qml/Download.qml", "qml/DownloadFinished.qml", "qml/HFBase/Error.qml" } };

void LauncherState::ASSERT_STATE(LauncherState::ApplicationState state) {
    if (_applicationState != state) {
#ifdef Q_OS_WIN
        __debugbreak();
#endif
        setApplicationState(ApplicationState::UnexpectedError);
    }
}

void LauncherState::ASSERT_STATE(std::vector<LauncherState::ApplicationState> states) {
    for (auto state : states) {
        if (_applicationState == state) {
            return;
        }
    }

#ifdef Q_OS_WIN
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
            return SPLASH_SCREEN;
        case ApplicationState::WaitingForLogin:
        case ApplicationState::RequestingLogin:
            return LOGIN_SCREEN;
        case ApplicationState::DownloadingClient:
        case ApplicationState::InstallingClient:
        case ApplicationState::DownloadingContentCache:
        case ApplicationState::InstallingContentCache:
            return DOWNLOAD_SCREEN;
        case ApplicationState::LaunchingHighFidelity:
            return DOWNLOAD_FINSISHED;
        case ApplicationState::UnexpectedError:
            #ifdef Q_OS_WIN
            __debugbreak();
            #endif
            return ERROR_SCREEN;
        default:
            qDebug() << "FATAL: No UI for" << _applicationState;
            #ifdef Q_OS_WIN
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

void LauncherState::requestBuilds() {
    ASSERT_STATE(ApplicationState::Init);
    setApplicationState(ApplicationState::RequestingBuilds);

    // TODO Show splash screen until this request is complete
    auto request = new QNetworkRequest(QUrl("https://thunder.highfidelity.com/builds/api/tags/latest/?format=json"));
    auto reply = _networkAccessManager.get(*request);

    QObject::connect(reply, &QNetworkReply::finished, this, &LauncherState::receivedBuildsReply);
}

void LauncherState::receivedBuildsReply() {
    auto reply = static_cast<QNetworkReply*>(sender());

    ASSERT_STATE(ApplicationState::RequestingBuilds);

    if (reply->error()) {
        qDebug() << "Error getting builds from thunder: " << reply->errorString();
    } else {
        qDebug() << "Builds reply has been received";
        auto data = reply->readAll();
        QJsonParseError parseError;
        auto doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error) {
            qDebug() << "Error parsing response from thunder: " << data;
        } else {
            auto root = doc.object();
            if (!root.contains("default_tag")) {
                setApplicationState(ApplicationState::UnexpectedError);
                return;
            }

            _latestBuilds.defaultTag = root["default_tag"].toString();

            auto results = root["results"];
            if (!results.isArray()) {
                setApplicationState(ApplicationState::UnexpectedError);
                return;
            }

            for (auto result : results.toArray()) {
                auto entry = result.toObject();
                Build build;
                build.tag = entry["name"].toString();
                build.latestVersion = entry["latest_version"].toInt();
                build.buildNumber = entry["build_number"].toInt();
#ifdef Q_OS_WIN
                build.installerZipURL = entry["installers"].toObject()["windows"].toObject()["zip_url"].toString();
#elif defined(Q_OS_MACOS)
                build.installerZipURL = entry["installers"].toObject()["mac"].toObject()["zip_url"].toString();
#else
                #error "Launcher is only supported on Windows and Mac OS"
#endif
                _latestBuilds.builds.push_back(build);
            }
        }
    }
    setApplicationState(ApplicationState::WaitingForLogin);
}

void LauncherState::login(QString username, QString password) {
    ASSERT_STATE(ApplicationState::WaitingForLogin);

    setApplicationState(ApplicationState::RequestingLogin);

    qDebug() << "Got login: " << username << password;

    auto request = new QNetworkRequest(QUrl("https://metaverse.highfidelity.com/oauth/token"));

    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery query;
    query.addQueryItem("grant_type", "password");
    query.addQueryItem("username", username);
    query.addQueryItem("password", password);
    query.addQueryItem("scope", "owner");

    auto reply = _networkAccessManager.post(*request, query.toString().toUtf8());
    QObject::connect(reply, &QNetworkReply::finished, this, &LauncherState::receivedLoginReply);
}

Q_INVOKABLE void LauncherState::receivedLoginReply() {
    ASSERT_STATE(ApplicationState::RequestingLogin);

    // TODO Check for errors
    auto reply = static_cast<QNetworkReply*>(sender());

    if (reply->error()) {
        setApplicationState(ApplicationState::UnexpectedError);
        return;
    }

    auto data = reply->readAll();
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(data, &parseError);
    auto root = doc.object();

    if (!root.contains("access_token")
        || !root.contains("token_type")
        || !root.contains("expires_in")
        || !root.contains("refresh_token")
        || !root.contains("scope")
        || !root.contains("created_at")) {

        setApplicationState(ApplicationState::UnexpectedError);
        return;
    }

    _loginResponse.accessToken = root["access_token"].toString();
    _loginResponse.refreshToken = root["refresh_token"].toString();
    _loginResponse.tokenType = root["token_type"].toString();

    qDebug() << "Got response for login: " << data;
    _loginTokenResponse = data;

    downloadClient();
}

QString LauncherState::getContentCachePath() const {
    return _launcherDirectory.filePath("cache");
}

bool LauncherState::shouldDownloadContentCache() const {
    return !_contentCacheURL.isNull() && !QFile::exists(getContentCachePath());
}

void LauncherState::downloadClient() {
    ASSERT_STATE(ApplicationState::RequestingLogin);

    Build build;
    if (!_latestBuilds.getBuild(_buildTag, &build)) {
        qDebug() << "Cannot determine latest build";
        setApplicationState(ApplicationState::UnexpectedError);
        return;
    }

    _downloadProgress = 0;
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
            _downloadProgress = (float)received / (float)total;
            emit downloadProgressChanged();
        });
    }
}

void LauncherState::clientDownloadComplete() {
    ASSERT_STATE(ApplicationState::DownloadingClient);

    _clientZipFile.close();

    installClient();
}

void LauncherState::installClient() {
    ASSERT_STATE(ApplicationState::DownloadingClient);
    setApplicationState(ApplicationState::InstallingClient);

    auto installDir = _launcherDirectory.absoluteFilePath("interface_install");
    _launcherDirectory.mkpath("interface_install");

    _downloadProgress = 0;

    qDebug() << "Unzipping " << _clientZipFile.fileName() << " to " << installDir;

    auto unzipper = new Unzipper(_clientZipFile.fileName(), QDir(installDir));
    unzipper->setAutoDelete(true);
    connect(unzipper, &Unzipper::progress, this, [this](float progress) {
        qDebug() << "Unzipper progress: " << progress;
        _downloadProgress = progress;
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

void LauncherState::downloadContentCache() {
    ASSERT_STATE(ApplicationState::InstallingClient);

    // Start content set cache download
    if (shouldDownloadContentCache()) {
        setApplicationState(ApplicationState::DownloadingContentCache);

        _downloadProgress = 0;

        auto request = new QNetworkRequest(QUrl(_contentCacheURL));
        auto reply = _networkAccessManager.get(*request);

        _contentZipFile.setFileName(_launcherDirectory.absoluteFilePath("content_cache.zip"));

        qDebug() << "Opening " << _contentZipFile.fileName();
        if (!_contentZipFile.open(QIODevice::WriteOnly)) {
            setApplicationState(ApplicationState::UnexpectedError);
            return;
        }

        connect(reply, &QNetworkReply::finished, this, &LauncherState::contentCacheDownloadComplete);
        connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
            char buf[4096];
            while (reply->bytesAvailable() > 0) {
                qint64 size;
                size = reply->read(buf, (qint64)sizeof(buf));
                if (size == 0) {
                    break;
                }
                _contentZipFile.write(buf, size);
            }
        });
        connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
            _downloadProgress = (float)received / (float)total;
            emit downloadProgressChanged();
        });
    } else {
        launchClient();
    }
}

void LauncherState::contentCacheDownloadComplete() {
    ASSERT_STATE(ApplicationState::DownloadingContentCache);

    _contentZipFile.close();

    installContentCache();
}


void LauncherState::installContentCache() {
    ASSERT_STATE(ApplicationState::DownloadingContentCache);
    setApplicationState(ApplicationState::InstallingContentCache);

    auto installDir = getContentCachePath();

    qDebug() << "Unzipping " << _contentZipFile.fileName() << " to " << installDir;

    _downloadProgress = 0;

    auto unzipper = new Unzipper(_contentZipFile.fileName(), QDir(installDir));
    unzipper->setAutoDelete(true);
    connect(unzipper, &Unzipper::progress, this, [this](float progress) {
        qDebug() << "Unzipper progress (content cache): " << progress;
        _downloadProgress = progress;
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
    ASSERT_STATE({ ApplicationState::InstallingClient, ApplicationState::InstallingContentCache });

    setApplicationState(ApplicationState::LaunchingHighFidelity);

    QDir installDirectory = _launcherDirectory.filePath("interface_install");
    QString clientPath;
#if defined(Q_OS_WIN)
    clientPath = installDirectory.absoluteFilePath("interface.exe");
#elif defined(Q_OS_MACOS)
    clientPath = installDirectory.absoluteFilePath("interface.app/Contents/MacOS/interface");
#endif

    QString homePath = "hifi://hq";
    QString defaultScriptsPath;
#if defined(Q_OS_WIN)
    defaultScriptsPath = installDirectory.filePath("scripts/simplifiedUIBootstrapper.js");
#elif defined(Q_OS_MACOS)
    defaultScriptsPath = installDirectory.filePath("interface.app/Contents/Resources/scripts/simplifiedUIBootstrapper.js");
#endif

    qDebug() << "------> " << defaultScriptsPath;
    QString displayName = "fixMe";
    QString contentCachePath = _launcherDirectory.filePath("cache");

    ::launchClient(clientPath, homePath, QDir::toNativeSeparators(defaultScriptsPath), displayName, contentCachePath, _loginTokenResponse);
}

void LauncherState::setApplicationState(ApplicationState state) {
    qDebug() << "Changing application state: " << _applicationState << " -> " << state;

    if (state == ApplicationState::UnexpectedError) {
        #ifdef Q_OS_WIN
        __debugbreak();
        #endif
    }

    _applicationState = state;

    emit uiStateChanged();
    emit updateSourceUrl(PathUtils::resourcePath(getCurrentUISource()));

    emit applicationStateChanged();
}

LauncherState::ApplicationState LauncherState::getApplicationState() const {
    return _applicationState;
}
