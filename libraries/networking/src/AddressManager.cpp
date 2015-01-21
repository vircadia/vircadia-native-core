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

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QJsonDocument>
#include <QRegExp>
#include <QStringList>

#include <GLMHelpers.h>
#include <Settings.h>
#include <UUID.h>

#include "NodeList.h"

#include "AddressManager.h"

const QString ADDRESS_MANAGER_SETTINGS_GROUP = "AddressManager";
const QString SETTINGS_CURRENT_ADDRESS_KEY = "address";
namespace SettingHandles {
    const SettingHandle<QUrl> currentAddress(QStringList() << ADDRESS_MANAGER_SETTINGS_GROUP
                                             << "address",
                                             QUrl());
}

AddressManager::AddressManager() :
    _rootPlaceName(),
    _rootPlaceID(),
    _positionGetter(NULL),
    _orientationGetter(NULL)
{
    connect(qApp, &QCoreApplication::aboutToQuit, this, &AddressManager::storeCurrentAddress);
}

bool AddressManager::isConnected() {
    return DependencyManager::get<NodeList>()->getDomainHandler().isConnected();
}

const QUrl AddressManager::currentAddress() const {
    QUrl hifiURL;
    
    hifiURL.setScheme(HIFI_URL_SCHEME);
    hifiURL.setHost(_rootPlaceName);
    hifiURL.setPath(currentPath());
    
    return hifiURL;
}

void AddressManager::loadSettings(const QString& lookupString) {
    if (lookupString.isEmpty()) {
        handleLookupString(SettingHandles::currentAddress.get().toString());
    } else {
        handleLookupString(lookupString);
    }
}

void AddressManager::storeCurrentAddress() {
    SettingHandles::currentAddress.set(currentAddress());
}

const QString AddressManager::currentPath(bool withOrientation) const {
    
    if (_positionGetter) {
        QString pathString = "/" + createByteArray(_positionGetter());
        
        if (withOrientation) {
            if (_orientationGetter) {
                QString orientationString = createByteArray(_orientationGetter());
                pathString += "/" + orientationString;
            } else {
                qDebug() << "Cannot add orientation to path without a getter for position."
                    << "Call AdressManager::setOrientationGetter to pass a function that will return a glm::quat";
            }
            
        }
        
        return pathString;
    } else {
        qDebug() << "Cannot create address path without a getter for position."
            << "Call AdressManager::setPositionGetter to pass a function that will return a const glm::vec3&";
        return QString();
    }
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
            if (handleNetworkAddress(lookupUrl.host()
                                      + (lookupUrl.port() == -1 ? "" : ":" + QString::number(lookupUrl.port())))) {
                // we may have a path that defines a relative viewpoint - if so we should jump to that now
                handleRelativeViewpoint(lookupUrl.path());
            } else {
                // wasn't an address - lookup the place name
                // we may have a path that defines a relative viewpoint - pass that through the lookup so we can go to it after
                attemptPlaceNameLookup(lookupUrl.host(), lookupUrl.path());
                
            }
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
        QString sanitizedString = lookupString.trimmed();
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

void AddressManager::handleAPIResponse(QNetworkReply& requestReply) {
    QJsonObject responseObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    QJsonObject dataObject = responseObject["data"].toObject();
    
    goToAddressFromObject(dataObject.toVariantMap(), requestReply);
    
    emit lookupResultsFinished();
}

const char OVERRIDE_PATH_KEY[] = "override_path";

void AddressManager::goToAddressFromObject(const QVariantMap& dataObject, const QNetworkReply& reply) {
    
    const QString DATA_OBJECT_PLACE_KEY = "place";
    const QString DATA_OBJECT_USER_LOCATION_KEY = "location";
    
    QVariantMap locationMap;
    if (dataObject.contains(DATA_OBJECT_PLACE_KEY)) {
        locationMap = dataObject[DATA_OBJECT_PLACE_KEY].toMap();
    } else {
        locationMap = dataObject[DATA_OBJECT_USER_LOCATION_KEY].toMap();
    }
    
    if (!locationMap.isEmpty()) {
        const QString LOCATION_API_ROOT_KEY = "root";
        const QString LOCATION_API_DOMAIN_KEY = "domain";
        const QString LOCATION_API_ONLINE_KEY = "online";
        
        if (!locationMap.contains(LOCATION_API_ONLINE_KEY)
            || locationMap[LOCATION_API_ONLINE_KEY].toBool()) {
            
            QVariantMap rootMap = locationMap[LOCATION_API_ROOT_KEY].toMap();
            if (rootMap.isEmpty()) {
                rootMap = locationMap;
            }
            
            QVariantMap domainObject = rootMap[LOCATION_API_DOMAIN_KEY].toMap();
            
            if (!domainObject.isEmpty()) {
                const QString DOMAIN_NETWORK_ADDRESS_KEY = "network_address";
                const QString DOMAIN_ICE_SERVER_ADDRESS_KEY = "ice_server_address";
                
                if (domainObject.contains(DOMAIN_NETWORK_ADDRESS_KEY)) {
                    QString domainHostname = domainObject[DOMAIN_NETWORK_ADDRESS_KEY].toString();
                    
                    qDebug() << "Possible domain change required to connect to" << domainHostname
                        << "on" << DEFAULT_DOMAIN_SERVER_PORT;
                    emit possibleDomainChangeRequired(domainHostname, DEFAULT_DOMAIN_SERVER_PORT);
                } else {
                    QString iceServerAddress = domainObject[DOMAIN_ICE_SERVER_ADDRESS_KEY].toString();
                    
                    const QString DOMAIN_ID_KEY = "id";
                    QString domainIDString = domainObject[DOMAIN_ID_KEY].toString();
                    QUuid domainID(domainIDString);
                    
                    qDebug() << "Possible domain change required to connect to domain with ID" << domainID
                        << "via ice-server at" << iceServerAddress;
                    
                    emit possibleDomainChangeRequiredViaICEForID(iceServerAddress, domainID);
                }
                
                // set our current root place id to the ID that came back
                const QString PLACE_ID_KEY = "id";
                _rootPlaceID = rootMap[PLACE_ID_KEY].toUuid();
                
                // set our current root place name to the name that came back
                const QString PLACE_NAME_KEY = "name";
                QString newRootPlaceName = rootMap[PLACE_NAME_KEY].toString();
                setRootPlaceName(newRootPlaceName);
                
                // check if we had a path to override the path returned
                QString overridePath = reply.property(OVERRIDE_PATH_KEY).toString();
                
                if (!overridePath.isEmpty()) {
                    if (!handleRelativeViewpoint(overridePath)){
                        qDebug() << "User entered path could not be handled as a viewpoint - " << overridePath;
                    }
                } else {
                    // take the path that came back
                    const QString PLACE_PATH_KEY = "path";
                    QString returnedPath = locationMap[PLACE_PATH_KEY].toString();
                    
                    bool shouldFaceViewpoint = locationMap.contains(LOCATION_API_ONLINE_KEY);
                    
                    if (!returnedPath.isEmpty()) {
                        // try to parse this returned path as a viewpoint, that's the only thing it could be for now
                        if (!handleRelativeViewpoint(returnedPath, shouldFaceViewpoint)) {
                            qDebug() << "Received a location path that was could not be handled as a viewpoint -" << returnedPath;
                        }
                    }
                }
                
                
            } else {
                qDebug() << "Received an address manager API response with no domain key. Cannot parse.";
                qDebug() << locationMap;
            }
        } else {
            // we've been told that this result exists but is offline, emit our signal so the application can handle
            emit lookupResultIsOffline();
        }
    } else {
        qDebug() << "Received an address manager API response with no location key or place key. Cannot parse.";
        qDebug() << locationMap;
    }
}

void AddressManager::handleAPIError(QNetworkReply& errorReply) {
    qDebug() << "AddressManager API error -" << errorReply.error() << "-" << errorReply.errorString();
    
    if (errorReply.error() == QNetworkReply::ContentNotFoundError) {
        emit lookupResultIsNotFound();
    }
    emit lookupResultsFinished();
}

const QString GET_PLACE = "/api/v1/places/%1";

void AddressManager::attemptPlaceNameLookup(const QString& lookupString, const QString& overridePath) {
    // assume this is a place name and see if we can get any info on it
    QString placeName = QUrl::toPercentEncoding(lookupString);
    
    QVariantMap requestParams;
    if (!overridePath.isEmpty()) {
        requestParams.insert(OVERRIDE_PATH_KEY, overridePath);
    }
    
    AccountManager::getInstance().unauthenticatedRequest(GET_PLACE.arg(placeName),
                                                         QNetworkAccessManager::GetOperation,
                                                         apiCallbackParameters(),
                                                         QByteArray(),
                                                         NULL,
                                                         requestParams);
}

bool AddressManager::handleNetworkAddress(const QString& lookupString) {
    const QString IP_ADDRESS_REGEX_STRING = "^((?:(?:[0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}"
        "(?:[0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))(?::(\\d{1,5}))?$";
    
    const QString HOSTNAME_REGEX_STRING = "^((?:[A-Z0-9]|[A-Z0-9][A-Z0-9\\-]{0,61}[A-Z0-9])"
        "(?:\\.(?:[A-Z0-9]|[A-Z0-9][A-Z0-9\\-]{0,61}[A-Z0-9]))+|localhost)(?::(\\d{1,5}))?$";
    
    QRegExp ipAddressRegex(IP_ADDRESS_REGEX_STRING);
    
    if (ipAddressRegex.indexIn(lookupString) != -1) {
        QString domainIPString = ipAddressRegex.cap(1);
        
        qint16 domainPort = DEFAULT_DOMAIN_SERVER_PORT;
        if (!ipAddressRegex.cap(2).isEmpty()) {
            domainPort = (qint16) ipAddressRegex.cap(2).toInt();
        }
        
        emit lookupResultsFinished();
        setDomainInfo(domainIPString, domainPort);
        
        return true;
    }
    
    QRegExp hostnameRegex(HOSTNAME_REGEX_STRING, Qt::CaseInsensitive);
    
    if (hostnameRegex.indexIn(lookupString) != -1) {
        QString domainHostname = hostnameRegex.cap(1);
        
        qint16 domainPort = DEFAULT_DOMAIN_SERVER_PORT;
        if (!hostnameRegex.cap(2).isEmpty()) {
            domainPort = (qint16) hostnameRegex.cap(2).toInt();
        }
        
        emit lookupResultsFinished();
        setDomainInfo(domainHostname, domainPort);
        
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

void AddressManager::setRootPlaceName(const QString& rootPlaceName) {
    if (rootPlaceName != _rootPlaceName) {
        _rootPlaceName = rootPlaceName;
        emit rootPlaceNameChanged(_rootPlaceName);
    }
}


void AddressManager::setDomainInfo(const QString& hostname, quint16 port) {
    _rootPlaceName = hostname;
    _rootPlaceID = QUuid();
    
    qDebug() << "Possible domain change required to connect to domain at" << hostname << "on" << port;
    
    emit possibleDomainChangeRequired(hostname, port);
}

void AddressManager::goToUser(const QString& username) {
    QString formattedUsername = QUrl::toPercentEncoding(username);
    // this is a username - pull the captured name and lookup that user's location
    AccountManager::getInstance().unauthenticatedRequest(GET_USER_LOCATION.arg(formattedUsername),
                                                         QNetworkAccessManager::GetOperation,
                                                         apiCallbackParameters());
}

void AddressManager::copyAddress() {
    QApplication::clipboard()->setText(currentAddress().toString());
}

void AddressManager::copyPath() {
    QApplication::clipboard()->setText(currentPath());
}
