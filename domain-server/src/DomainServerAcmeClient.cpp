//
//  DomainServerAcmeClient.cpp
//  domain-server/src
//
//  Created by Nshan G. on 2021-11-15.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainServerAcmeClient.h"

#include <HTTPManager.h>
#include <HTTPConnection.h>
#include <PathUtils.h>
#include <acme/acme-lw.hpp>
#include <SSLCommon.h>

#include <QDir>

#include <memory>
#include <array>

#include "DomainServerSettingsManager.h"


Q_LOGGING_CATEGORY(acme_client, "vircadia.acme_client")

using namespace std::literals;
using std::chrono::system_clock;

class AcmeHttpChallengeServer : public AcmeChallengeHandler, public HTTPRequestHandler {
    struct Challenge {
        QUrl url;
        QByteArray content;
    };

public:
    AcmeHttpChallengeServer() :
        manager(QHostAddress::AnyIPv4, 80, "", this)
    {}

    void addChallenge(const std::string&, const std::string& location, const std::string& content) override {
        challenges.push_back({
            QString::fromStdString(location),
            QByteArray::fromStdString(content)
        });
    }

    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false) override {
        auto challenge = std::find_if(challenges.begin(), challenges.end(), [&url](auto x) { return x.url == url; });
        if(challenge != challenges.end()) {
            connection->respond(HTTPConnection::StatusCode200, challenge->content, "application/octet-stream");
        } else {
            using namespace std::string_literals;
            auto chstr = ""s;
            for(auto&& ch : challenges) {
                chstr += ch.url.toString().toStdString();
                chstr += '\n';
            }
            connection->respond(HTTPConnection::StatusCode404, ("Resource not found. Url is "s + url.toString().toStdString() + " but expected any of\n"s + chstr).c_str());
        }
        return true;
    }

private:
    HTTPManager manager;
    std::vector<Challenge> challenges;
};

class AcmeHttpChallengeFiles : public AcmeChallengeHandler {

public:
    AcmeHttpChallengeFiles(const QString& rootPath) : challenges() {}

    ~AcmeHttpChallengeFiles() override {
        // TODO: delete directories/files
    }

    void addChallenge(const std::string& domain, const std::string& location, const std::string& content) override {
        challenges.push_back({
            QString::fromStdString(location),
        });
        // TODO: create directory/file, write content
    }

private:
    std::vector<QString> challenges;
};

class AcmeHttpChallengeManual : public AcmeChallengeHandler {
public:
    void addChallenge(const std::string& domain, const std::string& location, const std::string& content) override {
        qCDebug(acme_client) << "Please manually complete this http challenge:\n"
            << "Domain:" << domain.c_str() << '\n'
            << "Location:" << location.c_str() << '\n'
            << "Content:" << content.c_str() << '\n';
    }
};

template <typename Callback>
class ChallengeSelfCheck :
    public std::enable_shared_from_this< ChallengeSelfCheck<Callback> >
{
    public:
    ChallengeSelfCheck(Callback callback, std::vector<std::string> urls) :
        callback(std::move(callback)),
        urls(std::move(urls))
    {}

    void start() {
        for(auto&& url : urls) {
            acme_lw::waitForGet(shared_callback{this->shared_from_this()},
                std::move(url), 1s, 250ms);
                // TODO: 120s timeout 1s interval when using AcmeHttpChallengeManual
        }
    }

    ~ChallengeSelfCheck() {
        callback();
    }

    void operator()(acme_lw::Response) const {}

    void operator()(acme_lw::AcmeException error) const {
        qCWarning(acme_client) << "Challenge self-check failed: " << error.what() << '\n';
    }

    private:
    struct shared_callback {
        std::shared_ptr<ChallengeSelfCheck> ptr;
        template <typename... Args>
        void operator()(Args&&... args) {
            ptr->operator()(std::forward<Args>(args)...);
        }
    };

    Callback callback;
    std::vector<std::string> urls;

};

template <typename Callback>
auto challengeSelfCheck(Callback callback, std::vector<std::string> urls) {
    return std::make_shared<ChallengeSelfCheck<Callback>>(
        std::move(callback), std::move(urls));
}

bool createAccountKey(QFile& file) {
    if (file.open(QFile::WriteOnly)) {
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
        file.write(QByteArray::fromStdString(acme_lw::toPemString(acme_lw::makePrivateKey())));
        file.close();
        return true;
    }
    return false;
}

std::string readAll(const QString& path)
{
    QFile file(path);
    if(file.open(QFile::ReadOnly)) {
        return file.readAll().toStdString();
    }
    return std::string();
}

acme_lw::Certificate readCertificate(const std::array<QString, 2>& files) {
    return {
        readAll(files.front()),
        readAll(files.back()),
    };
}

bool mkpath(const QString& path) {
    return QDir(QFileInfo(path).path()).mkpath(".");
}

bool writeAll(const std::string& data, const QString& path)
{
    if (!mkpath(path)) {
        return false;
    }
    QFile file(path);
    return file.open(QFile::WriteOnly) &&
        file.write(QByteArray::fromStdString(data)) == qint64(data.size());
}

bool writeCertificate(acme_lw::Certificate cert, const std::array<QString, 2>& files) {
    return writeAll(cert.fullchain, files.front()) &&
        writeAll(cert.privkey, files.back());
}

CertificatePaths DomainServerAcmeClient::getCertificatePaths(DomainServerSettingsManager& settings) {
    auto certDirStr = settings.valueOrDefaultValueForKeyPath("acme.certificate_directory").toString();
    QDir certDir(certDirStr != ""
        ? QDir(certDirStr)
        : PathUtils::getAppLocalDataPath()
    );

    CertificatePaths paths {
        settings.valueOrDefaultValueForKeyPath("acme.certificate_filename").toString(),
        settings.valueOrDefaultValueForKeyPath("acme.certificate_key_filename").toString(),
        settings.valueOrDefaultValueForKeyPath("acme.certificate_authority_filename").toString()
    };

    paths.cert = certDir.filePath(paths.cert);
    paths.key = certDir.filePath(paths.key);
    paths.trustedAuthorities = certDir.filePath(paths.trustedAuthorities);
    return paths;
}

DomainServerAcmeClient::DomainServerAcmeClient(DomainServerSettingsManager& settings) :
    renewalTimer(),
    challengeHandler(nullptr),
    selfCheckUrls(),
    settings(settings)
{
    renewalTimer.setSingleShot(true);
    connect(&renewalTimer, &QTimer::timeout, this, [this](){ init(); } );
    init();
}

void DomainServerAcmeClient::init() {

    auto paths = DomainServerAcmeClient::getCertificatePaths(settings);
    std::array<QString,2> certPaths { paths.cert, paths.key };
    auto notExisitng = std::stable_partition(certPaths.begin(), certPaths.end(),
        [](auto x){ return QFile::exists(x); });
    if(notExisitng == certPaths.end()) {
        // all files exist, order unchanged
        checkExpiry(std::move(certPaths));
    } else if(notExisitng == certPaths.begin()) {
        // none of the files exist, order unchanged
        generateCertificate(std::move(certPaths));
    } else {
        // one file exist while the other doesn't, ordered existing first
        qCCritical(acme_client) << "SSL certificate missing file:\n" << *notExisitng;
        qCCritical(acme_client) << "Either provide it, or remove the other file to generate a new certificate:\n" << *certPaths.begin();
        return;
    }
}

system_clock::duration remainingTime(system_clock::time_point expiryTime) {
    return (expiryTime - system_clock::now()) * 2 / 3;
}

QDateTime dateTimeFrom(system_clock::time_point time) {
    QDateTime scheduleTime;
    auto secondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>
        (time.time_since_epoch());
    scheduleTime.setSecsSinceEpoch(secondsSinceEpoch.count());
    return scheduleTime;
}

template<typename Callback>
struct CertificateCallback{
    std::unique_ptr<AcmeChallengeHandler>& challengeHandler;
    std::array<QString,2> certPaths;
    Callback next;

    void operator()(acme_lw::AcmeClient client, acme_lw::Certificate cert) const {
        challengeHandler = nullptr;
        qCDebug(acme_client) << "Certificate retrieved\n"
            << "Expires on:" << dateTimeFrom(cert.getExpiry()) << '\n'
        ;
        if(writeCertificate(cert, certPaths)) {
            next(std::move(cert));
        } else {
            qCCritical(acme_client) << "Failed to write certificate files.\n"
                << certPaths.front() << '\n'
                << certPaths.back() << '\n';
        }
    }
    void operator()(acme_lw::AcmeClient client, acme_lw::AcmeException error) const {
        challengeHandler = nullptr;
        qCCritical(acme_client) << error.what() << '\n';
    }
};

template<typename Callback>
CertificateCallback<Callback> certificateCallback(
    std::unique_ptr<AcmeChallengeHandler>& challengeHandler,
    std::array<QString,2> certPaths,
    Callback next
) {
    return {
        challengeHandler,
        std::move(certPaths),
        std::move(next)
    };
}

template <typename Callback>
struct OrderCallback{
    std::unique_ptr<AcmeChallengeHandler>& challengeHandler;
    std::vector<std::string>& selfCheckUrls;
    std::array<QString,2> certPaths;
    Callback next;

    void operator()(acme_lw::AcmeClient client, std::vector<std::string> challenges, std::vector<std::string> domains, std::string finalUrl, std::string orderUrl) const {
        qCDebug(acme_client) << "Ordered certificate\n"
            << "Order URL:" << orderUrl.c_str() << '\n'
            << "Finalize URL:" << finalUrl.c_str() << '\n'
            << "Number of domains:" << domains.size() << '\n'
            << "Number of challenges:" << challenges.size() << '\n'
        ;
        challengeSelfCheck([client = std::move(client), orderUrl, finalUrl, challenges, domains, &challengeHandler = challengeHandler, certPaths = std::move(certPaths), next = std::move(next)]() mutable {
            retrieveCertificate(certificateCallback(challengeHandler,std::move(certPaths), std::move(next)),
                std::move(client), std::move(domains), std::move(challenges),
                std::move(orderUrl), std::move(finalUrl)
            );
        }, std::move(selfCheckUrls))->start();
        selfCheckUrls.clear();
    }
    void operator()(acme_lw::AcmeClient client, acme_lw::AcmeException error) const {
        qCCritical(acme_client) << error.what() << '\n';
    }
    void operator()(acme_lw::AcmeException error) const {
        qCCritical(acme_client) << error.what() << '\n';
    }
};

template<typename Callback>
OrderCallback<Callback> orderCallback(
    std::unique_ptr<AcmeChallengeHandler>& challengeHandler,
    std::vector<std::string>& selfCheckUrls,
    std::array<QString,2> certPaths,
    Callback next
) {
    return {
        challengeHandler,
        selfCheckUrls,
        std::move(certPaths),
        std::move(next)
    };
}

// TODO: on failure retry N times then reschedule for next day
void DomainServerAcmeClient::generateCertificate(std::array<QString,2> certPaths) {

    QString accountKeyPath = settings.valueOrDefaultValueForKeyPath("acme.account_key_path").toString();
    if(accountKeyPath == "") {
        accountKeyPath = QDir(PathUtils::getAppLocalDataPath()).filePath("acme_account_key.pem");
    }
    QFile accountKeyFile(accountKeyPath);
    if(!accountKeyFile.exists()) {
        if(!mkpath(accountKeyPath) || !createAccountKey(accountKeyFile)) {
            qCCritical(acme_client) << "Failed to create account key file " << accountKeyFile.fileName();
            return;
        }
    }

    std::string accountKey = "";
    if(accountKeyFile.open(QFile::ReadOnly))
    {
        accountKey = accountKeyFile.readAll().toStdString();
        accountKeyFile.close();
    } else {
        qCCritical(acme_client) << "Failed to read account key file " << accountKeyFile.fileName();
        return;
    }

    std::vector<std::string> domains;
    auto domainList = settings.valueOrDefaultValueForKeyPath("acme.certificate_domains").toList();
    for(auto&& var : domainList) {
        domains.push_back(QUrl::toAce(var.toString()).toStdString());
    }

    auto directoryUrl = settings.valueOrDefaultValueForKeyPath("acme.directory_endpoint").toString().toStdString();

    acme_lw::init(acme_lw::forwardAcmeError([this, domains = std::move(domains)](auto next, auto client){
        acme_lw::createAccount(acme_lw::forwardAcmeError([this, domains = std::move(domains)](auto next, auto client){
            // TODO: add configuration settings for AcmeHttpChallengeFiles or AcmeHttpChallengeManual
            challengeHandler = std::make_unique<AcmeHttpChallengeServer>();
            acme_lw::orderCertificate(std::move(next), [this](auto domain, auto location, auto keyAuth){
                qCDebug(acme_client) << "Got challenge:\n"
                    << "Domain:" << domain.c_str() << '\n'
                    << "Location:" << location.c_str() << '\n'
                    << "Key Authorization:" << keyAuth.c_str() << '\n'
                ;
                challengeHandler->addChallenge(domain, location, keyAuth);
                selfCheckUrls.push_back("http://"s + domain + location);
            }, std::move(client), std::move(domains));
        }, std::move(next)), std::move(client));
    }, orderCallback(challengeHandler, selfCheckUrls, std::move(certPaths), [this](auto cert){
        scheduleRenewalIn(remainingTime(cert.getExpiry()));
    })), accountKey, directoryUrl);

}

void DomainServerAcmeClient::checkExpiry(std::array<QString,2> certPaths) {
    auto cert = readCertificate(certPaths);
    if(cert.fullchain.empty() || cert.privkey.empty()) {
        // TODO: report a proper error, IO error, bad certificate etc
        qCCritical(acme_client) << "Failed to read certificate files.\n"
            << certPaths.front() << '\n'
            << certPaths.back() << '\n';
        return;
    }

    auto remaining = remainingTime(cert.getExpiry());
    if(remaining > 0s) {
        scheduleRenewalIn(remaining);
    } else {
        generateCertificate(std::move(certPaths));
    }
}

void DomainServerAcmeClient::scheduleRenewalIn(system_clock::duration duration) {
    renewalTimer.stop();
    renewalTimer.start(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    qCDebug(acme_client) << "Renewal scheduled for:" << dateTimeFrom(system_clock::now() + duration);
}

bool DomainServerAcmeClient::handleAuthenticatedHTTPRequest(HTTPConnection *connection, const QUrl &url) {
    // TODO: report current status, upload private key and certificate files
    return false;
}
