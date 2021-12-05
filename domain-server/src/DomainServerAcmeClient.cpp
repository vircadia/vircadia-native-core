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

#include <memory>

#include "acme/acme-lw.hpp"

Q_LOGGING_CATEGORY(acme_client, "vircadia.acme_client")

using namespace std::literals;

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
    AcmeHttpChallengeFiles(const QString rootPath) : challenges() {}

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

template <typename Callback>
class ChallengeSelfCheck :
    public std::enable_shared_from_this< ChallengeSelfCheck<Callback> >
{

    struct shared_callback {
        std::shared_ptr<ChallengeSelfCheck> ptr;
        template <typename... Args>
        void operator()(Args&&... args) {
            ptr->operator()(std::forward<Args>(args)...);
        }
    };

    Callback callback;
    std::vector<std::string> urls;

    public:
    ChallengeSelfCheck(Callback callback, std::vector<std::string> urls) :
        callback(std::move(callback)),
        urls(std::move(urls))
    {}

    void start() {
        for(auto&& url : urls) {
            acme_lw::waitForGet(shared_callback{this->shared_from_this()},
                std::move(url), 1s, 250ms);
        }
    }

    ~ChallengeSelfCheck() {
        callback();
    }

    void operator()(acme_lw::Response) const {}

    void operator()(acme_lw::AcmeException error) const {
        qCWarning(acme_client) << "Challenge self-check failed: " << error.what() << '\n';
    }
};

DomainServerAcmeClient::DomainServerAcmeClient() :
    expiryTimer(),
    challengeHandler(nullptr)
{

    struct CertificateCallback{
        std::unique_ptr<AcmeChallengeHandler>& challengeHandler;

        void operator()(acme_lw::AcmeClient client, acme_lw::Certificate cert) const {
            challengeHandler = nullptr;
            qCDebug(acme_client) << "Certificate retrieved\n"
                << "Expires on:" << cert.getExpiryDisplay().c_str() << '\n'
            ;
        }
        void operator()(acme_lw::AcmeClient client, acme_lw::AcmeException error) const {
            qCCritical(acme_client) << error.what() << '\n';
        }
    };

    struct OrderCallback{
        std::unique_ptr<AcmeChallengeHandler>& challengeHandler;
        std::vector<std::string>& selfCheckUrls;

        void operator()(acme_lw::AcmeClient client, std::vector<std::string> challenges, std::vector<std::string> domains, std::string finalUrl, std::string orderUrl) const {
            qCDebug(acme_client) << "Ordered certificate\n"
                << "Order URL:" << orderUrl.c_str() << '\n'
                << "Finalize URL:" << finalUrl.c_str() << '\n'
                << "Number of domains:" << domains.size() << '\n'
                << "Number of challenges:" << challenges.size() << '\n'
            ;
            auto afterSelfCheck = [client = std::move(client), orderUrl, finalUrl, challenges, domains, this]() mutable {
                retrieveCertificate(CertificateCallback{challengeHandler},
                    std::move(client), std::move(domains), std::move(challenges),
                    std::move(orderUrl), std::move(finalUrl)
                );
            };
            std::make_shared<ChallengeSelfCheck<decltype(afterSelfCheck)>>
                (std::move(afterSelfCheck), std::move(selfCheckUrls))->start();
            selfCheckUrls.clear();
        }
        void operator()(acme_lw::AcmeClient client, acme_lw::AcmeException error) const {
            qCCritical(acme_client) << error.what() << '\n';
        }
        void operator()(acme_lw::AcmeException error) const {
            qCCritical(acme_client) << error.what() << '\n';
        }
    };

    acme_lw::init(acme_lw::forwardAcmeError([this](auto next, auto client){
        acme_lw::createAccount(acme_lw::forwardAcmeError([this](auto next, auto client){
            // TODO: conditionally use AcmeHttpChallengeFiles
            challengeHandler = std::make_unique<AcmeHttpChallengeServer>();
            acme_lw::orderCertificate(std::move(next), [this](auto domain, auto location, auto keyAuth){
                qCDebug(acme_client) << "Got challenge:\n"
                    << "Domain:" << domain.c_str() << '\n'
                    << "Location:" << location.c_str() << '\n'
                    << "Key Authorization:" << keyAuth.c_str() << '\n'
                ;
                challengeHandler->addChallenge(domain, location, keyAuth);
                selfCheckUrls.push_back("http://"s + domain + location);
            }, std::move(client), {"example.com"});
        }, std::move(next)), std::move(client));
    }, OrderCallback{challengeHandler, selfCheckUrls}), acme_lw::toPemString(acme_lw::makePrivateKey()));
}

void DomainServerAcmeClient::certificateExpiryTimerHandler() {
}

bool DomainServerAcmeClient::handleAuthenticatedHTTPRequest(HTTPConnection *connection, const QUrl &url) {
    return false;
}
