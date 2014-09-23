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
        QString orientationString = createByteArray(orientation);
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

bool AddressManager::handleUrl(const QUrl& lookupUrl) {
    if (lookupUrl.scheme() == HIFI_URL_SCHEME) {
        
        qDebug() << "Trying to go to URL" << lookupUrl.toString();
        
        // there are 4 possible lookup strings
        
        // 1. global place name (name of domain or place) - example: sanfrancisco
        // 2. user name (prepended with @) - example: @philip
        // 3. location string (posX,posY,posZ/eulerX,eulerY,eulerZ)
        // 4. domain network address (IP or dns resolvable hostname)
        
        // use our regex'ed helpers to figure out what we're supposed to do with this
        if (!handleUsername(lookupUrl.authority())) {
            // we're assuming this is either a network address or global place name
            // check if it is a network address first
            if (!handleNetworkAddress(lookupUrl.host())) {
                // wasn't an address - lookup the place name
                attemptPlaceNameLookup(lookupUrl.host());
            }
            
            // we may have a path that defines a relative viewpoint - if so we should jump to that now
            handleRelativeViewpoint(lookupUrl.path());
        }
        
        return true;
    } else if (lookupUrl.toString().startsWith('/')) {
        qDebug() << "Going to relative path" << lookupUrl.path();
        
        // if this is a relative path then handle it as a relative viewpoint
        handleRelativeViewpoint(lookupUrl.path());
        emit lookupResultsFinished();
    }
    
    return false;
}

void AddressManager::handleLookupString(const QString& lookupString) {
    if (!lookupString.isEmpty()) {
        // make this a valid hifi URL and handle it off to handleUrl
        QString sanitizedString = lookupString;
        QUrl lookupURL;
        
        if (!lookupString.startsWith('/')) {
            const QRegExp HIFI_SCHEME_REGEX = QRegExp(HIFI_URL_SCHEME + ":\\/{1,2}", Qt::CaseInsensitive);
            sanitizedString = sanitizedString.remove(HIFI_SCHEME_REGEX);
            
            lookupURL = QUrl(HIFI_URL_SCHEME + "://" + sanitizedString);
        } else {
            lookupURL = QUrl(lookupString);
        }
        
        handleUrl(lookupURL);
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
            
            bool shouldFaceViewpoint = dataObject.contains(ADDRESS_API_ONLINE_KEY);
            
            if (!returnedPath.isEmpty()) {
                // try to parse this returned path as a viewpoint, that's the only thing it could be for now
                if (!handleRelativeViewpoint(returnedPath, shouldFaceViewpoint)) {
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
    emit lookupResultsFinished();
}

void AddressManager::handleAPIError(QNetworkReply& errorReply) {
    qDebug() << "AddressManager API error -" << errorReply.error() << "-" << errorReply.errorString();
    
    if (errorReply.error() == QNetworkReply::ContentNotFoundError) {
        emit lookupResultIsNotFound();
    }
    emit lookupResultsFinished();
}

const QString GET_PLACE = "/api/v1/places/%1";

void AddressManager::attemptPlaceNameLookup(const QString& lookupString) {
    // assume this is a place name and see if we can get any info on it
    QString placeName = QUrl::toPercentEncoding(lookupString);
    AccountManager::getInstance().unauthenticatedRequest(GET_PLACE.arg(placeName),
                                                       QNetworkAccessManager::GetOperation,
                                                       apiCallbackParameters());
}

bool AddressManager::handleNetworkAddress(const QString& lookupString) {
    const QString IP_ADDRESS_REGEX_STRING = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}"
        "([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(:\\d{1,5})?$";
    
    const QString HOSTNAME_REGEX_STRING = "^((?:[A-Z0-9]|[A-Z0-9][A-Z0-9\\-]{0,61}[A-Z0-9])"
        "(?:\\.(?:[A-Z0-9]|[A-Z0-9][A-Z0-9\\-]{0,61}[A-Z0-9]))+|localhost)(:{1}\\d{1,5})?$";
    
    QRegExp hostnameRegex(HOSTNAME_REGEX_STRING, Qt::CaseInsensitive);
    
    if (hostnameRegex.indexIn(lookupString) != -1) {
        emit possibleDomainChangeRequired(hostnameRegex.cap(0));
        emit lookupResultsFinished();
        return true;
    }
    
    QRegExp ipAddressRegex(IP_ADDRESS_REGEX_STRING);
    
    if (ipAddressRegex.indexIn(lookupString) != -1) {
        emit possibleDomainChangeRequired(ipAddressRegex.cap(0));
        emit lookupResultsFinished();
        return true;
    }
    
    return false;
}

bool AddressManager::handleRelativeViewpoint(const QString& lookupString, bool shouldFace) {
    const QString FLOAT_REGEX_STRING = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)";
    const QString SPACED_COMMA_REGEX_STRING = "\\s*,\\s*";
    const QString POSITION_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
        FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + "\\s*(?:$|\\/)";
    const QString QUAT_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
        FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
        FLOAT_REGEX_STRING + "\\s*$";
    
    QRegExp positionRegex(POSITION_REGEX_STRING);
    
    if (positionRegex.indexIn(lookupString) != -1) {
        // we have at least a position, so emit our signal to say we need to change position
        glm::vec3 newPosition(positionRegex.cap(1).toFloat(),
                              positionRegex.cap(2).toFloat(),
                              positionRegex.cap(3).toFloat());
        
        if (!isNaN(newPosition.x) && !isNaN(newPosition.y) && !isNaN(newPosition.z)) {
            glm::quat newOrientation;
            
            QRegExp orientationRegex(QUAT_REGEX_STRING);
            
            // we may also have an orientation
            if (lookupString[positionRegex.matchedLength() - 1] == QChar('/')
                && orientationRegex.indexIn(lookupString, positionRegex.matchedLength() - 1) != -1) {
                
                glm::quat newOrientation = glm::normalize(glm::quat(orientationRegex.cap(4).toFloat(),
                                                                    orientationRegex.cap(1).toFloat(),
                                                                    orientationRegex.cap(2).toFloat(),
                                                                    orientationRegex.cap(3).toFloat()));
                
                if (!isNaN(newOrientation.x) && !isNaN(newOrientation.y) && !isNaN(newOrientation.z)
                    && !isNaN(newOrientation.w)) {
                    emit locationChangeRequired(newPosition, true, newOrientation, shouldFace);
                    return true;
                } else {
                    qDebug() << "Orientation parsed from lookup string is invalid. Will not use for location change.";
                }
            }
            
            emit locationChangeRequired(newPosition, false, newOrientation, shouldFace);
            
        } else {
            qDebug() << "Could not jump to position from lookup string because it has an invalid value.";
        }
        
        return true;
    }
    
    return false;
}

const QString GET_USER_LOCATION = "/api/v1/users/%1/location";

bool AddressManager::handleUsername(const QString& lookupString) {
    const QString USERNAME_REGEX_STRING = "^@(\\S+)";
    
    QRegExp usernameRegex(USERNAME_REGEX_STRING);
    
    if (usernameRegex.indexIn(lookupString) != -1) {
        goToUser(usernameRegex.cap(1));
        return true;
    }
    
    return false;
}

void AddressManager::goToUser(const QString& username) {
    QString formattedUsername = QUrl::toPercentEncoding(username);
    // this is a username - pull the captured name and lookup that user's location
    AccountManager::getInstance().unauthenticatedRequest(GET_USER_LOCATION.arg(formattedUsername),
                                                       QNetworkAccessManager::GetOperation,
                                                       apiCallbackParameters());
}
