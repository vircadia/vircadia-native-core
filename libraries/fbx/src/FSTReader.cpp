//
//  FSTReader.cpp
//
//
//  Created by Clement on 3/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QBuffer>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <SharedUtil.h>

#include "FSTReader.h"

QVariantHash FSTReader::parseMapping(QIODevice* device) {
    QVariantHash properties;
    
    QByteArray line;
    while (!(line = device->readLine()).isEmpty()) {
        if ((line = line.trimmed()).startsWith('#')) {
            continue; // comment
        }
        QList<QByteArray> sections = line.split('=');
        if (sections.size() < 2) {
            continue;
        }
        QByteArray name = sections.at(0).trimmed();
        if (sections.size() == 2) {
            properties.insertMulti(name, sections.at(1).trimmed());
            
        } else if (sections.size() == 3) {
            QVariantHash heading = properties.value(name).toHash();
            heading.insertMulti(sections.at(1).trimmed(), sections.at(2).trimmed());
            properties.insert(name, heading);
            
        } else if (sections.size() >= 4) {
            QVariantHash heading = properties.value(name).toHash();
            QVariantList contents;
            for (int i = 2; i < sections.size(); i++) {
                contents.append(sections.at(i).trimmed());
            }
            heading.insertMulti(sections.at(1).trimmed(), contents);
            properties.insert(name, heading);
        }
    }
    
    return properties;
}

QVariantHash FSTReader::readMapping(const QByteArray& data) {
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    return FSTReader::parseMapping(&buffer);
}

void FSTReader::writeVariant(QBuffer& buffer, QVariantHash::const_iterator& it) {
    QByteArray key = it.key().toUtf8() + " = ";
    QVariantHash hashValue = it.value().toHash();
    if (hashValue.isEmpty()) {
        buffer.write(key + it.value().toByteArray() + "\n");
        return;
    }
    for (QVariantHash::const_iterator second = hashValue.constBegin(); second != hashValue.constEnd(); second++) {
        QByteArray extendedKey = key + second.key().toUtf8();
        QVariantList listValue = second.value().toList();
        if (listValue.isEmpty()) {
            buffer.write(extendedKey + " = " + second.value().toByteArray() + "\n");
            continue;
        }
        buffer.write(extendedKey);
        for (QVariantList::const_iterator third = listValue.constBegin(); third != listValue.constEnd(); third++) {
            buffer.write(" = " + third->toByteArray());
        }
        buffer.write("\n");
    }
}

QByteArray FSTReader::writeMapping(const QVariantHash& mapping) {
    static const QStringList PREFERED_ORDER = QStringList() << NAME_FIELD << TYPE_FIELD << SCALE_FIELD << FILENAME_FIELD
    << TEXDIR_FIELD << JOINT_FIELD << FREE_JOINT_FIELD
    << BLENDSHAPE_FIELD << JOINT_INDEX_FIELD;
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    
    for (auto key : PREFERED_ORDER) {
        auto it = mapping.find(key);
        if (it != mapping.constEnd()) {
            if (key == FREE_JOINT_FIELD) { // writeVariant does not handle strings added using insertMulti.
                for (auto multi : mapping.values(key)) {
                    buffer.write(key.toUtf8());
                    buffer.write(" = ");
                    buffer.write(multi.toByteArray());
                    buffer.write("\n");
                }
            } else {
                writeVariant(buffer, it);
            }
        }
    }
    
    for (auto it = mapping.constBegin(); it != mapping.constEnd(); it++) {
        if (!PREFERED_ORDER.contains(it.key())) {
            writeVariant(buffer, it);
        }
    }
    return buffer.data();
}

QHash<FSTReader::ModelType, QString> FSTReader::_typesToNames;
QString FSTReader::getNameFromType(ModelType modelType) {
    if (_typesToNames.size() == 0) {
        _typesToNames[ENTITY_MODEL] = "entity";
        _typesToNames[HEAD_MODEL] = "head";
        _typesToNames[BODY_ONLY_MODEL] = "body";
        _typesToNames[HEAD_AND_BODY_MODEL] = "body+head";
        _typesToNames[ATTACHMENT_MODEL] = "attachment";
    }
    return _typesToNames[modelType];
}

QHash<QString, FSTReader::ModelType> FSTReader::_namesToTypes;
FSTReader::ModelType FSTReader::getTypeFromName(const QString& name) {
    if (_namesToTypes.size() == 0) {
        _namesToTypes["entity"] = ENTITY_MODEL;
        _namesToTypes["head"] = HEAD_MODEL ;
        _namesToTypes["body"] = BODY_ONLY_MODEL;
        _namesToTypes["body+head"] = HEAD_AND_BODY_MODEL;
        
        // NOTE: this is not yet implemented, but will be used to allow you to attach fully independent models to your avatar
        _namesToTypes["attachment"] = ATTACHMENT_MODEL; 
    }
    return _namesToTypes[name];
}

FSTReader::ModelType FSTReader::predictModelType(const QVariantHash& mapping) {

    QVariantHash joints;

    if (mapping.contains("joint") && mapping["joint"].type() == QVariant::Hash) {
        joints = mapping["joint"].toHash();
    }

    // if the mapping includes the type hint... then we trust the mapping
    if (mapping.contains(TYPE_FIELD)) {
        return FSTReader::getTypeFromName(mapping[TYPE_FIELD].toString());
    }
    
    // check for blendshapes
    bool hasBlendshapes = mapping.contains(BLENDSHAPE_FIELD);

    // a Head needs to have these minimum fields...
    //joint = jointEyeLeft = EyeL = 1
    //joint = jointEyeRight = EyeR = 1
    //joint = jointNeck = Head = 1
    bool hasHeadMinimum = joints.contains("jointNeck") && joints.contains("jointEyeLeft") && joints.contains("jointEyeRight");

    // a Body needs to have these minimum fields...
    //joint = jointRoot = Hips
    //joint = jointLean = Spine
    //joint = jointNeck = Neck
    //joint = jointHead = HeadTop_End

    bool hasBodyMinimumJoints = joints.contains("jointRoot") && joints.contains("jointLean") && joints.contains("jointNeck")  
                                        && joints.contains("jointHead");
    
    bool isLikelyHead = hasBlendshapes || hasHeadMinimum;

    if (isLikelyHead && hasBodyMinimumJoints) {
        return HEAD_AND_BODY_MODEL;
    }
      
    if (isLikelyHead) {
        return HEAD_MODEL;
    }

    if (hasBodyMinimumJoints) {
        return BODY_ONLY_MODEL;
    }
    
    return ENTITY_MODEL;
}

QVariantHash FSTReader::downloadMapping(const QString& url) {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest = QNetworkRequest(url);
    networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(networkRequest);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QByteArray fstContents = reply->readAll();
    delete reply;
    return FSTReader::readMapping(fstContents);
}
