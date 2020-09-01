#pragma once

#include <QObject>
#include <QNetworkAccessManager>

struct Build {
    QString tag;
    int latestVersion{ 0 };
    int buildNumber{ 0 };
    QString installerZipURL;
};

struct Builds {
    bool getBuild(QString tag, Build* outBuild);

    QString defaultTag;
    std::vector<Build> builds;
    Build launcherBuild;
};

class BuildsRequest : public QObject {
    Q_OBJECT
public:
    enum class State {
        Unsent,
        Sending,
        Finished
    };

    enum class Error {
        None = 0,
        Unknown,
        MalformedResponse,
        MissingDefaultTag,
    };
    Q_ENUM(Error)

    void send(QNetworkAccessManager& nam);
    Error getError() const { return _error; }

    const Builds& getLatestBuilds() const { return _latestBuilds; }

signals:
    void finished();

private slots:
    void receivedResponse();

private:
    State _state { State::Unsent };
    Error _error { Error::None };

    Builds _latestBuilds;
};
