//
//  DomainServerAcmeClient.h
//  domain-server/src
//
//  Created by Nshan G. on 2021-11-15.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef VIRCADIA_DOMAIN_SERVER_SRC_DOMAINSERVERACMECLIENT_H
#define VIRCADIA_DOMAIN_SERVER_SRC_DOMAINSERVERACMECLIENT_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QLoggingCategory>
#include <nlohmann/json.hpp>

Q_DECLARE_LOGGING_CATEGORY(acme_client)

class HTTPConnection;
class DomainServerSettingsManager;
struct CertificatePaths;

class AcmeChallengeHandler {
public:
    virtual void addChallenge(const std::string& domain, const std::string& location, const std::string& content) = 0;
    virtual std::chrono::milliseconds selfCheckDuration() = 0;
    virtual std::chrono::milliseconds selfCheckInterval() = 0;
    virtual ~AcmeChallengeHandler(){};
};

class DomainServerAcmeClient : public QObject {
    Q_OBJECT
public:
    static CertificatePaths getCertificatePaths(DomainServerSettingsManager&);
    static QString getAccountKeyPath(DomainServerSettingsManager&);

    DomainServerAcmeClient(DomainServerSettingsManager&);
    bool handleAuthenticatedHTTPRequest(HTTPConnection* connection, const QUrl& url);

signals:
    void certificateUpdated(CertificatePaths);

private:
    void init();
    void generateCertificate(const CertificatePaths& certPaths);
    void checkExpiry(const CertificatePaths& certPaths);
    void handleRenewal(std::chrono::system_clock::time_point, const CertificatePaths& certPaths);
    void scheduleRenewalIn(std::chrono::system_clock::duration);

    QTimer renewalTimer;
    std::unique_ptr<AcmeChallengeHandler> challengeHandler;
    DomainServerSettingsManager& settings;
    nlohmann::json status;

    QTimer updateCheckTimer;
    std::chrono::system_clock::time_point expiry;
};

#endif /* end of include guard */
