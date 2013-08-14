//
//  JurisdictionMap.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 8/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "JurisdictionMap.h"
#include "VoxelNode.h"


// standard assignment
// copy assignment 
JurisdictionMap& JurisdictionMap::operator=(const JurisdictionMap& other) {
    copyContents(other);
    //printf("JurisdictionMap COPY ASSIGNMENT %p from %p\n", this, &other);
    return *this;
}

#ifdef HAS_MOVE_SEMANTICS
// Move constructor
JurisdictionMap::JurisdictionMap(JurisdictionMap&& other) : _rootOctalCode(NULL) {
    init(other._rootOctalCode, other._endNodes);
    other._rootOctalCode = NULL;
    other._endNodes.clear();
    //printf("JurisdictionMap MOVE CONSTRUCTOR %p from %p\n", this, &other);
}

// move assignment
JurisdictionMap& JurisdictionMap::operator=(JurisdictionMap&& other) {
    init(other._rootOctalCode, other._endNodes);
    other._rootOctalCode = NULL;
    other._endNodes.clear();
    //printf("JurisdictionMap MOVE ASSIGNMENT %p from %p\n", this, &other);
    return *this;
}
#endif

// Copy constructor
JurisdictionMap::JurisdictionMap(const JurisdictionMap& other) : _rootOctalCode(NULL) {
    copyContents(other);
    //printf("JurisdictionMap COPY CONSTRUCTOR %p from %p\n", this, &other);
}

void JurisdictionMap::copyContents(unsigned char* rootCodeIn, const std::vector<unsigned char*> endNodesIn) {
    unsigned char* rootCode;
    std::vector<unsigned char*> endNodes;
    if (rootCodeIn) {
        int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(rootCodeIn));
        rootCode = new unsigned char[bytes];
        memcpy(rootCode, rootCodeIn, bytes);
    } else {
        rootCode = new unsigned char[1];
        *rootCode = 0;
    }
    
    for (int i = 0; i < endNodesIn.size(); i++) {
        if (endNodesIn[i]) {
            int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(endNodesIn[i]));
            unsigned char* endNodeCode = new unsigned char[bytes];
            memcpy(endNodeCode, endNodesIn[i], bytes);
            endNodes.push_back(endNodeCode);
        }
    }
    init(rootCode, endNodes);
}

void JurisdictionMap::copyContents(const JurisdictionMap& other) {
    copyContents(other._rootOctalCode, other._endNodes);
}

JurisdictionMap::~JurisdictionMap() {
    clear();
    //printf("JurisdictionMap DESTRUCTOR %p\n",this);
}

void JurisdictionMap::clear() {
    if (_rootOctalCode) {
        delete[] _rootOctalCode;
        _rootOctalCode = NULL;
    }
    
    for (int i = 0; i < _endNodes.size(); i++) {
        if (_endNodes[i]) {
            delete[] _endNodes[i];
        }
    }
    _endNodes.clear();
}

JurisdictionMap::JurisdictionMap() : _rootOctalCode(NULL) {
    unsigned char* rootCode = new unsigned char[1];
    *rootCode = 0;
    
    std::vector<unsigned char*> emptyEndNodes;
    init(rootCode, emptyEndNodes);
    //printf("JurisdictionMap DEFAULT CONSTRUCTOR %p\n",this);
}

JurisdictionMap::JurisdictionMap(const char* filename) : _rootOctalCode(NULL) {
    clear(); // clean up our own memory
    readFromFile(filename);
    //printf("JurisdictionMap INI FILE CONSTRUCTOR %p\n",this);
}

JurisdictionMap::JurisdictionMap(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes)  
    : _rootOctalCode(NULL) {
    init(rootOctalCode, endNodes);
    //printf("JurisdictionMap OCTCODE CONSTRUCTOR %p\n",this);
}

JurisdictionMap::JurisdictionMap(const char* rootHexCode, const char* endNodesHexCodes) {
    _rootOctalCode = hexStringToOctalCode(QString(rootHexCode));
    
    QString endNodesHexStrings(endNodesHexCodes);
    QString delimiterPattern(",");
    QStringList endNodeList = endNodesHexStrings.split(delimiterPattern);

    for (int i = 0; i < endNodeList.size(); i++) {
        QString endNodeHexString = endNodeList.at(i);

        unsigned char* endNodeOctcode = hexStringToOctalCode(endNodeHexString);
        //printOctalCode(endNodeOctcode);
        _endNodes.push_back(endNodeOctcode);
    }    
    //printf("JurisdictionMap HEX STRING CONSTRUCTOR %p\n",this);
}


void JurisdictionMap::init(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes) {
    clear(); // clean up our own memory
    _rootOctalCode = rootOctalCode;
    _endNodes = endNodes;
}

JurisdictionMap::Area JurisdictionMap::isMyJurisdiction(unsigned char* nodeOctalCode, int childIndex) const {
    // to be in our jurisdiction, we must be under the root...

    // if the node is an ancestor of my root, then we return ABOVE
    if (isAncestorOf(nodeOctalCode, _rootOctalCode)) {
        return ABOVE;
    }
    
    // otherwise...
    bool isInJurisdiction = isAncestorOf(_rootOctalCode, nodeOctalCode, childIndex);
    // if we're under the root, then we can't be under any of the endpoints
    if (isInJurisdiction) {
        for (int i = 0; i < _endNodes.size(); i++) {
            bool isUnderEndNode = isAncestorOf(_endNodes[i], nodeOctalCode);
            if (isUnderEndNode) {
                isInJurisdiction = false;
                break;
            }
        }
    }
    return isInJurisdiction ? WITHIN : BELOW;
}


bool JurisdictionMap::readFromFile(const char* filename) {
    QString     settingsFile(filename);
    QSettings   settings(settingsFile, QSettings::IniFormat);
    QString     rootCode = settings.value("root","00").toString();
    qDebug() << "rootCode=" << rootCode << "\n";

    _rootOctalCode = hexStringToOctalCode(rootCode);
    printOctalCode(_rootOctalCode);

    settings.beginGroup("endNodes");
    const QStringList childKeys = settings.childKeys();
    QHash<QString, QString> values;
    foreach (const QString &childKey, childKeys) {
        QString childValue = settings.value(childKey).toString();
        values.insert(childKey, childValue);
        qDebug() << childKey << "=" << childValue << "\n";

        unsigned char* octcode = hexStringToOctalCode(childValue);
        printOctalCode(octcode);
        
        _endNodes.push_back(octcode);
    }
    settings.endGroup();
    return true;
}

bool JurisdictionMap::writeToFile(const char* filename) {
    QString     settingsFile(filename);
    QSettings   settings(settingsFile, QSettings::IniFormat);


    QString rootNodeValue = octalCodeToHexString(_rootOctalCode);

    settings.setValue("root", rootNodeValue);
    
    settings.beginGroup("endNodes");
    for (int i = 0; i < _endNodes.size(); i++) {
        QString key = QString("endnode%1").arg(i);
        QString value = octalCodeToHexString(_endNodes[i]);
        settings.setValue(key, value);
    }
    settings.endGroup();
    return true;
}
