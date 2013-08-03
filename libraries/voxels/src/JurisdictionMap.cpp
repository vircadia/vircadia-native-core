//
//  JurisdictionMap.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 8/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QSettings>
#include <QString>
#include <QStringList>

#include "JurisdictionMap.h"
#include "VoxelNode.h"

JurisdictionMap::~JurisdictionMap() {
    clear();
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
}

JurisdictionMap::JurisdictionMap(const char* filename) : _rootOctalCode(NULL) {
    clear(); // clean up our own memory
    readFromFile(filename);
}

JurisdictionMap::JurisdictionMap(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes)  
    : _rootOctalCode(NULL) {
    init(rootOctalCode, endNodes);
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
