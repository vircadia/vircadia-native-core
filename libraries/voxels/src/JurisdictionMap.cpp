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
    printf("JurisdictionMap& JurisdictionMap::operator=(JurisdictionMap const& other) COPY ASSIGNMENT %p from %p\n", this, &other);
    return *this;
}

// move assignment
JurisdictionMap& JurisdictionMap::operator=(JurisdictionMap&& other) {
    init(other._rootOctalCode, other._endNodes);
    other._rootOctalCode = NULL;
    other._endNodes.clear();
    printf("JurisdictionMap& JurisdictionMap::operator=(JurisdictionMap&& other) MOVE ASSIGNMENT %p from %p\n", this, &other);
    return *this;
}

// Move constructor
JurisdictionMap::JurisdictionMap(JurisdictionMap&& other) : _rootOctalCode(NULL) {
    init(other._rootOctalCode, other._endNodes);
    other._rootOctalCode = NULL;
    other._endNodes.clear();
    printf("JurisdictionMap::JurisdictionMap(JurisdictionMap&& other) MOVE CONSTRUCTOR %p from %p\n", this, &other);
}

// Copy constructor
JurisdictionMap::JurisdictionMap(const JurisdictionMap& other) : _rootOctalCode(NULL) {
    copyContents(other);
    printf("JurisdictionMap::JurisdictionMap(const JurisdictionMap& other) COPY CONSTRUCTOR %p from %p\n", this, &other);
}

void JurisdictionMap::copyContents(const JurisdictionMap& other) {
    unsigned char* rootCode;
    std::vector<unsigned char*> endNodes;
    if (other._rootOctalCode) {
        int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(other._rootOctalCode));
        rootCode = new unsigned char[bytes];
        memcpy(rootCode, other._rootOctalCode, bytes);
    } else {
        rootCode = new unsigned char[1];
        *rootCode = 0;
    }
    
    for (int i = 0; i < other._endNodes.size(); i++) {
        if (other._endNodes[i]) {
            int bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(other._endNodes[i]));
            unsigned char* endNodeCode = new unsigned char[bytes];
            memcpy(endNodeCode, other._endNodes[i], bytes);
            endNodes.push_back(endNodeCode);
        }
    }
    init(rootCode, endNodes);
}

JurisdictionMap::~JurisdictionMap() {
    clear();
    printf("JurisdictionMap::~JurisdictionMap() DESTRUCTOR %p\n",this);
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
    printf("JurisdictionMap::~JurisdictionMap() DEFAULT CONSTRUCTOR %p\n",this);
}

JurisdictionMap::JurisdictionMap(const char* filename) : _rootOctalCode(NULL) {
    clear(); // clean up our own memory
    readFromFile(filename);
    printf("JurisdictionMap::~JurisdictionMap() INI FILE CONSTRUCTOR %p\n",this);
}

JurisdictionMap::JurisdictionMap(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes)  
    : _rootOctalCode(NULL) {
    init(rootOctalCode, endNodes);
    printf("JurisdictionMap::~JurisdictionMap() OCTCODE CONSTRUCTOR %p\n",this);
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
    printf("JurisdictionMap::~JurisdictionMap() HEX STRING CONSTRUCTOR %p\n",this);
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

    //printf("isInJurisdiction=%s rootOctalCode=",debug::valueOf(isInJurisdiction));
    //printOctalCode(_rootOctalCode);
    //printf("nodeOctalCode=");
    //printOctalCode(nodeOctalCode);
    
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


unsigned char* JurisdictionMap::hexStringToOctalCode(const QString& input) const {
    const int HEX_NUMBER_BASE = 16;
    const int HEX_BYTE_SIZE = 2;
    int stringIndex = 0;
    int byteArrayIndex = 0;

    // allocate byte array based on half of string length
    unsigned char* bytes = new unsigned char[(input.length()) / HEX_BYTE_SIZE];
    
    // loop through the string - 2 bytes at a time converting
    //  it to decimal equivalent and store in byte array
    bool ok;
    while (stringIndex < input.length()) {
        uint value = input.mid(stringIndex, HEX_BYTE_SIZE).toUInt(&ok, HEX_NUMBER_BASE);
        if (!ok) {
            break;
        }
        bytes[byteArrayIndex] = (unsigned char)value;
        stringIndex += HEX_BYTE_SIZE;
        byteArrayIndex++;
    }
    
    // something went wrong
    if (!ok) {
        delete[] bytes;
        return NULL;
    }
    return bytes;
}

QString JurisdictionMap::octalCodeToHexString(unsigned char* octalCode) const {
    const int HEX_NUMBER_BASE = 16;
    const int HEX_BYTE_SIZE = 2;
    QString output;
    if (!octalCode) {
        output = "00";
    } else {
        for (int i = 0; i < bytesRequiredForCodeLength(*octalCode); i++) {
            output.append(QString("%1").arg(octalCode[i], HEX_BYTE_SIZE, HEX_NUMBER_BASE, QChar('0')).toUpper());
        }
    }
    return output;
}
