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

void JurisdictionMap::init(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes) {
    clear(); // clean up our own memory
    _rootOctalCode = rootOctalCode;
    _endNodes = endNodes;
}

bool JurisdictionMap::isMyJurisdiction(VoxelNode* node, int childIndex) const {
    return true;
}


bool JurisdictionMap::readFromFile(const char* filename) {
    QString     settingsFile(filename);
    QSettings   settings(settingsFile, QSettings::IniFormat);
    QString     rootCode = settings.value("root","").toString();
    qDebug() << "rootCode=" << rootCode << "\n";

    unsigned char* rootOctCode = hexStringToOctalCode(rootCode);
    printOctalCode(rootOctCode);

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

    settings.setValue("root", "rootNodeValue");
    
    settings.beginGroup("endNodes");
    for (int i = 0; i < _endNodes.size(); i++) {
        QString key = QString("endnode%1").arg(i);
        QString value = QString("valuenode%1").arg(i);
        settings.setValue(key, value);
    }
    settings.endGroup();
    return true;
}


unsigned char* JurisdictionMap::hexStringToOctalCode(const QString& input) {
    // i variable used to hold position in string
    int i = 0;
    // x variable used to hold byte array element position
    int x = 0;
    // allocate byte array based on half of string length
    unsigned char* bytes = new unsigned char[(input.length()) / 2];
    // loop through the string - 2 bytes at a time converting
    //  it to decimal equivalent and store in byte array
    while (input.length() > i + 1) {
    
        bool ok;
        uint value = input.mid(i, 2).toUInt(&ok,16); 
        bytes[x] = (unsigned char)value;
        i += 2;
        x += 1;
    }
    // return the finished byte array of decimal values
    return bytes;
}