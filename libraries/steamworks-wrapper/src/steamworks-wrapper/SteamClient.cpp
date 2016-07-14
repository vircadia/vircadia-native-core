//
//  SteamClient.cpp
//  steamworks-wrapper/src/steamworks-wrapper
//
//  Created by Clement Brisset on 6/8/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SteamClient.h"

#include <atomic>
#include <memory>

#include <QtCore/QDebug>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include <steam/steam_api.h>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif


static const Ticket INVALID_TICKET = Ticket();

class SteamTicketRequests {
public:
    SteamTicketRequests();
    ~SteamTicketRequests();

    HAuthTicket startRequest(TicketRequestCallback callback);
    void stopRequest(HAuthTicket authTicket);

    STEAM_CALLBACK(SteamTicketRequests, onGetAuthSessionTicketResponse,
                   GetAuthSessionTicketResponse_t, _getAuthSessionTicketResponse);

private:
    void stopAll();

    struct PendingTicket {
        HAuthTicket authTicket;
        Ticket ticket;
        TicketRequestCallback callback;
    };

    std::vector<PendingTicket> _pendingTickets;
};

SteamTicketRequests::SteamTicketRequests() :
    _getAuthSessionTicketResponse(this, &SteamTicketRequests::onGetAuthSessionTicketResponse)
{
}

SteamTicketRequests::~SteamTicketRequests() {
    stopAll();
}

HAuthTicket SteamTicketRequests::startRequest(TicketRequestCallback callback) {
    static const uint32 MAX_TICKET_SIZE { 1024 };
    uint32 ticketSize { 0 };
    char ticket[MAX_TICKET_SIZE];

    auto authTicket = SteamUser()->GetAuthSessionTicket(ticket, MAX_TICKET_SIZE, &ticketSize);
    qDebug() << "Got Steam auth session ticket:" << authTicket;

    if (authTicket == k_HAuthTicketInvalid) {
        qWarning() << "Auth session ticket is invalid.";
        callback(INVALID_TICKET);
    } else {
        PendingTicket pendingTicket{ authTicket, QByteArray(ticket, ticketSize).toHex(), callback };
        _pendingTickets.push_back(pendingTicket);
    }

    return authTicket;
}

void SteamTicketRequests::stopRequest(HAuthTicket authTicket) {
    auto it = std::find_if(_pendingTickets.begin(), _pendingTickets.end(), [&authTicket](const PendingTicket& pendingTicket) {
        return pendingTicket.authTicket == authTicket;
    });

    if (it != _pendingTickets.end()) {
        SteamUser()->CancelAuthTicket(it->authTicket);
        it->callback(INVALID_TICKET);
        _pendingTickets.erase(it);
    }
}

void SteamTicketRequests::stopAll() {
    auto steamUser = SteamUser();
    if (steamUser) {
        for (const auto& pendingTicket : _pendingTickets) {
            steamUser->CancelAuthTicket(pendingTicket.authTicket);
            pendingTicket.callback(INVALID_TICKET);
        }
    }
    _pendingTickets.clear();
}

void SteamTicketRequests::onGetAuthSessionTicketResponse(GetAuthSessionTicketResponse_t* pCallback) {
    auto authTicket = pCallback->m_hAuthTicket;

    auto it = std::find_if(_pendingTickets.begin(), _pendingTickets.end(), [&authTicket](const PendingTicket& pendingTicket) {
        return pendingTicket.authTicket == authTicket;
    });


    if (it != _pendingTickets.end()) {

        if (pCallback->m_eResult == k_EResultOK) {
            qDebug() << "Got steam callback, auth session ticket is valid. Send it." << authTicket;
            it->callback(it->ticket);
        } else {
            qWarning() << "Steam auth session ticket callback encountered an error:" << pCallback->m_eResult;
            it->callback(INVALID_TICKET);
        }

        _pendingTickets.erase(it);
    } else {
        qWarning() << "Could not find steam auth session ticket in list of pending tickets:" << authTicket;
    }
}



static std::atomic_bool initialized { false };
static std::unique_ptr<SteamTicketRequests> steamTicketRequests;

bool SteamClient::init() {
    if (!initialized) {
        initialized = SteamAPI_Init();
    }

    if (!steamTicketRequests && initialized) {
        steamTicketRequests.reset(new SteamTicketRequests());
    }

    return initialized;
}

void SteamClient::shutdown() {
    if (initialized) {
        SteamAPI_Shutdown();
    }

    if (steamTicketRequests) {
        steamTicketRequests.reset();
    }
}

void SteamClient::runCallbacks() {
    if (!initialized) {
        init();
    }

    if (!initialized) {
        qDebug() << "Steam not initialized";
        return;
    }

    auto steamPipe = SteamAPI_GetHSteamPipe();
    if (!steamPipe) {
        qDebug() << "Could not get SteamPipe";
        return;
    }

    Steam_RunCallbacks(steamPipe, false);
}

void SteamClient::requestTicket(TicketRequestCallback callback) {
    if (!initialized) {
        init();
    }

    if (!initialized) {
        qDebug() << "Steam not initialized";
        return;
    }

    steamTicketRequests->startRequest(callback);
}


