#include "BuildsRequest.h"

#include "Helper.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcessEnvironment>

bool Builds::getBuild(QString tag, Build* outBuild) {
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

void BuildsRequest::send(QNetworkAccessManager& nam) {
    QString latestBuildRequestUrl { "https://thunder.highfidelity.com/builds/api/tags/latest/?format=json" };
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();

    if (processEnvironment.contains("HQ_LAUNCHER_BUILDS_URL")) {
        latestBuildRequestUrl = processEnvironment.value("HQ_LAUNCHER_BUILDS_URL");
    }

    qDebug() << latestBuildRequestUrl;

    QNetworkRequest request{ QUrl(latestBuildRequestUrl) };
    auto reply = nam.get(request);

    QObject::connect(reply, &QNetworkReply::finished, this, &BuildsRequest::receivedResponse);
}

void BuildsRequest::receivedResponse() {
    _state = State::Finished;

    auto reply = static_cast<QNetworkReply*>(sender());

    if (reply->error()) {
        qDebug() << "Error getting builds from thunder: " << reply->errorString();
        _error = Error::Unknown;
        emit finished();
        return;
    } else {
        qDebug() << "Builds reply has been received";

        auto data = reply->readAll();

        QJsonParseError parseError;
        auto doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error) {
            qDebug() << "Error parsing response from thunder: " << data;
            _error = Error::Unknown;
        } else {
            auto root = doc.object();
            if (!root.contains("default_tag")) {
                _error = Error::MissingDefaultTag;
                emit finished();
                return;
            }

            _latestBuilds.defaultTag = root["default_tag"].toString();

            auto results = root["results"];
            if (!results.isArray()) {
                _error = Error::MalformedResponse;
                emit finished();
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

            auto launcherResults = root["launcher"].toObject();

            Build launcherBuild;
            launcherBuild.latestVersion = launcherResults["version"].toInt();

#ifdef Q_OS_WIN
            launcherBuild.installerZipURL = launcherResults["windows"].toObject()["url"].toString();
#elif defined(Q_OS_MACOS)
            launcherBuild.installerZipURL = launcherResults["mac"].toObject()["url"].toString();
#else
#error "Launcher is only supported on Windows and Mac OS"
#endif
            _latestBuilds.launcherBuild = launcherBuild;
        }
    }


    emit finished();
}
