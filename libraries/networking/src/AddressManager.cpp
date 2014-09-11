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

#include "AddressManager.h"

AddressManager& AddressManager::getInstance() {
    static AddressManager sharedInstance;
    return sharedInstance;
}

void AddressManager::handleLookupString(const QString& lookupString) {
    // there are 4 possible lookup strings
    
    // 1. global place name (name of domain or place) - example: sanfrancisco
    // 2. user name (prepended with @) - example: @philip
    // 3. location string (posX,posY,posZ/eulerX,eulerY,eulerZ)
    // 4. domain network address (IP or dns resolvable hostname)
    
    // use our regex'ed helpers to figure out what we're supposed to do with this
    if (!lookupHandledAsUsername(lookupString) &&
        !lookupHandledAsNetworkAddress(lookupString) &&
        !lookupHandledAsLocationString(lookupString)) {
        
    }

    
   
}

bool AddressManager::lookupHandledAsNetworkAddress(const QString& lookupString) {
    const QString IP_ADDRESS_REGEX_STRING = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}"
        "([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(:\\d{1,5})?$";
    
    const QString HOSTNAME_REGEX_STRING = "^(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])"
        "(?:\\.(?:[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]))+(:{1}\\d{1,5})?$";
    
    QRegExp hostnameRegex(HOSTNAME_REGEX_STRING);
    
    if (hostnameRegex.indexIn(lookupString) != -1) {
        emit domainChangeRequired(hostnameRegex.cap(0));
        return true;
    }
    
    QRegExp ipAddressRegex(IP_ADDRESS_REGEX_STRING);
    
    if (ipAddressRegex.indexIn(lookupString) != -1) {
        emit domainChangeRequired(ipAddressRegex.cap(0));
        return true;
    }
    
    return false;
}

bool AddressManager::lookupHandledAsLocationString(const QString& lookupString) {
    const QString FLOAT_REGEX_STRING = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)";
    const QString TRIPLE_FLOAT_REGEX_STRING = FLOAT_REGEX_STRING + "\\s*,\\s*" +
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

bool AddressManager::lookupHandledAsUsername(const QString& lookupString) {
    const QString USERNAME_REGEX_STRING = "^@(\\S+)$";
    
    QRegExp usernameRegex(USERNAME_REGEX_STRING);
    
    if (usernameRegex.indexIn(lookupString) != -1) {
        // this is a username - pull the captured name and lookup that user's address
        
        return true;
    }
    
    return false;
}
