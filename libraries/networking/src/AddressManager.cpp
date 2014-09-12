//
//  AddressManager.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-09-10.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qdebug.h>
#include <qregexp.h>
#include <qstringlist.h>

#include <GLMHelpers.h>

#include "AddressManager.h"

AddressManager& AddressManager::getInstance() {
    static AddressManager sharedInstance;
    return sharedInstance;
}

QString AddressManager::pathForPositionAndOrientation(const glm::vec3& position, bool hasOrientation,
                                                      const glm::quat& orientation) {
    
    QString pathString = "/" + createByteArray(position);
    
    if (hasOrientation) {
        QString orientationString = createByteArray(glm::degrees(safeEulerAngles(orientation)));
        pathString += "/" + orientationString;
    }
    
    return pathString;
}

const JSONCallbackParameters& AddressManager::apiCallbackParameters() {
    static bool hasSetupParameters = false;
    static JSONCallbackParameters callbackParams;
    
    if (!hasSetupParameters) {
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "handleAPIResponse";
        callbackParams.errorCallbackReceiver = this;
        callbackParams.errorCallbackMethod = "handleAPIError";
    }
    
    return callbackParams;
}

void AddressManager::handleLookupString(const QString& lookupString) {
    // there are 4 possible lookup strings
    
    // 1. global place name (name of domain or place) - example: sanfrancisco
    // 2. user name (prepended with @) - example: @philip
    // 3. location string (posX,posY,posZ/eulerX,eulerY,eulerZ)
    // 4. domain network address (IP or dns resolvable hostname)
    
    QString sanitizedLookupString = lookupString.trimmed().remove(HIFI_URL_SCHEME + "//");
    
    
    // use our regex'ed helpers to figure out what we're supposed to do with this
    if (!isLookupHandledAsUsername(sanitizedLookupString) &&
        !isLookupHandledAsNetworkAddress(sanitizedLookupString) &&
        !isLookupHandledAsViewpoint(sanitizedLookupString)) {
        attemptPlaceNameLookup(sanitizedLookupString);
    }
}

void AddressManager::handleAPIResponse(const QJsonObject &jsonObject) {
    QJsonObject dataObject = jsonObject["data"].toObject();
    
    const QString ADDRESS_API_DOMAIN_KEY = "domain";
    const QString ADDRESS_API_ONLINE_KEY = "online";
    
    if (!dataObject.contains(ADDRESS_API_ONLINE_KEY)
        || dataObject[ADDRESS_API_ONLINE_KEY].toBool()) {
        
        if (dataObject.contains(ADDRESS_API_DOMAIN_KEY)) {
            QJsonObject domainObject = dataObject[ADDRESS_API_DOMAIN_KEY].toObject();
            
            const QString DOMAIN_NETWORK_ADDRESS_KEY = "network_address";
            QString domainHostname = domainObject[DOMAIN_NETWORK_ADDRESS_KEY].toString();
            
            emit possibleDomainChangeRequired(domainHostname);
            
            // take the path that came back
            const QString LOCATION_KEY = "location";
            const QString LOCATION_PATH_KEY = "path";
            QString returnedPath;
            
            if (domainObject.contains(LOCATION_PATH_KEY)) {
                returnedPath = domainObject[LOCATION_PATH_KEY].toString();
            } else if (domainObject.contains(LOCATION_KEY)) {
                returnedPath = domainObject[LOCATION_KEY].toObject()[LOCATION_PATH_KEY].toString();
            }
            
            if (!returnedPath.isEmpty()) {
                // try to parse this returned path as a viewpoint, that's the only thing it could be for now
                if (!isLookupHandledAsViewpoint(returnedPath)) {
                    qDebug() << "Received a location path that was could not be handled as a viewpoint -" << returnedPath;
                }
            }
            
        } else {
            qDebug() << "Received an address manager API response with no domain key. Cannot parse.";
            qDebug() << jsonObject;
        }
    } else {
        // we've been told that this result exists but is offline, emit our signal so the application can handle
        emit lookupResultIsOffline();
    }
}

void AddressManager::handleAPIError(QNetworkReply::NetworkError error, const QString& message) {
    qDebug() << "AddressManager API error -" << error << "-" << message;
}

const QString GET_PLACE = "/api/v1/places/%1";

void AddressManager::attemptPlaceNameLookup(const QString& lookupString) {
    // assume this is a place name and see if we can get any info on it
    QString placeName = QUrl::toPercentEncoding(lookupString);
    AccountManager::getInstance().authenticatedRequest(GET_PLACE.arg(placeName),
                                                       QNetworkAccessManager::GetOperation,
                                                       apiCallbackParameters());
}

bool AddressManager::isLookupHandledAsNetworkAddress(const QString& lookupString) {
    const QString IP_ADDRESS_REGEX_STRING = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}"
        "([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(:\\d{1,5})?$";
    
    const QString HOSTNAME_REGEX_STRING = "^((?:[A-Z0-9]|[A-Z0-9][A-Z0-9\\-]{0,61}[A-Z0-9])"
        "(?:\\.(?:[A-Z0-9]|[A-Z0-9][A-Z0-9\\-]{0,61}[A-Z0-9]))+|localhost)(:{1}\\d{1,5})?$";
    
    QRegExp hostnameRegex(HOSTNAME_REGEX_STRING, Qt::CaseInsensitive);
    
    if (hostnameRegex.indexIn(lookupString) != -1) {
        emit possibleDomainChangeRequired(hostnameRegex.cap(0));
        return true;
    }
    
    QRegExp ipAddressRegex(IP_ADDRESS_REGEX_STRING);
    
    if (ipAddressRegex.indexIn(lookupString) != -1) {
        emit possibleDomainChangeRequired(ipAddressRegex.cap(0));
        return true;
    }
    
    return false;
}

bool AddressManager::isLookupHandledAsViewpoint(const QString& lookupString) {
    const QString FLOAT_REGEX_STRING = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)";
    const QString TRIPLE_FLOAT_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + "\\s*,\\s*" +
    FLOAT_REGEX_STRING + "\\s*,\\s*" + FLOAT_REGEX_STRING + "\\s*(?:$|\\/)";
    
    QRegExp tripleFloatRegex(TRIPLE_FLOAT_REGEX_STRING);
    
    if (tripleFloatRegex.indexIn(lookupString) != -1) {
        // we have at least a position, so emit our signal to say we need to change position
        glm::vec3 newPosition(tripleFloatRegex.cap(1).toFloat(),
                              tripleFloatRegex.cap(2).toFloat(),
                              tripleFloatRegex.cap(3).toFloat());
        
        if (newPosition.x != NAN && newPosition.y != NAN &&  newPosition.z != NAN) {
            glm::vec3 newOrientation;
            // we may also have an orientation
            if (lookupString[tripleFloatRegex.matchedLength() - 1] == QChar('/')
                && tripleFloatRegex.indexIn(lookupString, tripleFloatRegex.matchedLength() - 1) != -1) {
                
                glm::vec3 newOrientation(tripleFloatRegex.cap(1).toFloat(),
                                         tripleFloatRegex.cap(2).toFloat(),
                                         tripleFloatRegex.cap(3).toFloat());
                
                if (newOrientation.x != NAN && newOrientation.y != NAN && newOrientation.z != NAN) {
                    emit locationChangeRequired(newPosition, true, newOrientation);
                    return true;
                } else {
                    qDebug() << "Orientation parsed from lookup string is invalid. Will not use for location change.";
                }
            }
            
            emit locationChangeRequired(newPosition, false, newOrientation);
            
        } else {
            qDebug() << "Could not jump to position from lookup string because it has an invalid value.";
        }
        
        return true;
    }
    
    return false;
}

const QString GET_USER_LOCATION = "/api/v1/users/%1/location";

bool AddressManager::isLookupHandledAsUsername(const QString& lookupString) {
    const QString USERNAME_REGEX_STRING = "^@(\\S+)$";
    
    QRegExp usernameRegex(USERNAME_REGEX_STRING);
    
    if (usernameRegex.indexIn(lookupString) != -1) {
        QString username = QUrl::toPercentEncoding(usernameRegex.cap(1));
        // this is a username - pull the captured name and lookup that user's location
        AccountManager::getInstance().authenticatedRequest(GET_USER_LOCATION.arg(username),
                                                           QNetworkAccessManager::GetOperation,
                                                           apiCallbackParameters());
        
        return true;
    }
    
    return false;
}
