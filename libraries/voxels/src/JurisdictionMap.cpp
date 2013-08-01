//
//  JurisdictionMap.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 8/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QSettings>

#include "JurisdictionMap.h"
#include "VoxelNode.h"

JurisdictionMap::~JurisdictionMap() {
    clear();
}

void JurisdictionMap::clear() {
    delete[] _rootOctalCode;
    _rootOctalCode = NULL;
    
    for (int i = 0; i < _endNodes.size(); i++) {
        delete[] _endNodes[i];
    }
    _endNodes.clear();
}

JurisdictionMap::JurisdictionMap() {
    unsigned char* rootCode = new unsigned char[1];
    *rootCode = 0;
    
    std::vector<unsigned char*> emptyEndNodes;
    init(rootCode, emptyEndNodes);
}

JurisdictionMap::JurisdictionMap(const char* filename) {
    clear(); // clean up our own memory
    readFromFile(filename);
}


JurisdictionMap::JurisdictionMap(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes) {
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
    QSettings settings(QString(filename), QSettings::IniFormat);
    QString rootCode = settings->value("root","").toString();
    qDebug("rootCode=%s\n",rootCode);
}

bool JurisdictionMap::writeToFile(const char* filename) {
}
