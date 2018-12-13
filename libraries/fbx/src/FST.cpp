//
//  FST.cpp
//
//  Created by Ryan Huffman on 12/11/15.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "FST.h"

#include <QDir>
#include <QFileInfo>

FST::FST(QString fstPath, QVariantHash data) : _fstPath(fstPath) {
    if (data.contains("name")) {
        _name = data["name"].toString(); 
        data.remove("name");
    }

    if (data.contains("filename")) {
        _modelPath = data["filename"].toString(); 
        data.remove("filename");
    }

    _other = data;
}

QString FST::absoluteModelPath() const {
    QFileInfo fileInfo{ _fstPath };
    QDir dir{ fileInfo.absoluteDir() };
    return dir.absoluteFilePath(_modelPath);
}

void FST::setName(const QString& name) {
    _name = name;
    emit nameChanged(name);
}

void FST::setModelPath(const QString& modelPath) {
    _modelPath = modelPath;
    emit modelPathChanged(modelPath);
}