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

#include "acme/acme-lw.hpp"

Q_LOGGING_CATEGORY(acme_client, "vircadia.acme.client")

int i = 0;

DomainServerAcmeClient::DomainServerAcmeClient() {

    struct clientCallback{
        void operator()(acme_lw::AcmeClient client, std::vector<std::string> challenges, std::vector<std::string> domains, std::string finalUrl, std::string orderUrl) const {
            qCDebug(acme_client) << "Ordered certificate\n"
                << "Order URL:" << orderUrl.c_str() << '\n'
                << "Finalize URL:" << finalUrl.c_str() << '\n'
                << "Number of challenges:" << challenges.size() << '\n'
            ;
        }
        void operator()(acme_lw::AcmeClient client, acme_lw::AcmeException error) const {
            qCCritical(acme_client) << error.what() << '\n';
        }
        void operator()(acme_lw::AcmeException error) const {
            qCCritical(acme_client) << error.what() << '\n';
        }
    };
    acme_lw::init(acme_lw::forwardAcmeError([](auto next, auto client){
        acme_lw::createAccount(acme_lw::forwardAcmeError([](auto next, auto client){
            acme_lw::orderCertificate(std::move(next), [](auto domain, auto url, auto keyAuth){
                qCDebug(acme_client) << "Got challenge:\n"
                    << "Domain:" << domain.c_str() << '\n'
                    << "URL:" << url.c_str() << '\n'
                    << "Key Authorization:" << keyAuth.c_str() << '\n'
                ;
            }, std::move(client), {"example.com"});
        }, std::move(next)), std::move(client));
    }, clientCallback{}), acme_lw::toPemString(acme_lw::makePrivateKey()));
}

void DomainServerAcmeClient::certificateExpiryTimerHandler() {
}

bool DomainServerAcmeClient::handleAuthenticatedHTTPRequest(HTTPConnection *connection, const QUrl &url) {
    return false;
}
