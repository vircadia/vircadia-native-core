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


DomainServerAcmeClient::DomainServerAcmeClient() {

    struct {
        void operator()(acme_lw::AcmeClient client) const {
            std::cout << "got client!!!!!!!!!!!!!!" << '\n';
        }
        void operator()(acme_lw::AcmeException error) const {
            throw error;
        }
    } initCallback;
    acme_lw::init(initCallback, acme_lw::toPemString(acme_lw::makePrivateKey()));
}

void DomainServerAcmeClient::certificateExpiryTimerHandler() {
}

bool DomainServerAcmeClient::handleAuthenticatedHTTPRequest(HTTPConnection *connection, const QUrl &url) {
    return false;
}
