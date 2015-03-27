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

#include "FSTReader.h"

QVariantHash parseMapping(QIODevice* device) {
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

QVariantHash readMapping(const QByteArray& data) {
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    return parseMapping(&buffer);
}

void writeVariant(QBuffer& buffer, QVariantHash::const_iterator& it) {
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
};

QByteArray writeMapping(const QVariantHash& mapping) {
    static const QStringList PREFERED_ORDER = QStringList() << NAME_FIELD << SCALE_FIELD << FILENAME_FIELD
    << TEXDIR_FIELD << JOINT_FIELD << FREE_JOINT_FIELD
    << BLENDSHAPE_FIELD << JOINT_INDEX_FIELD;
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    
    for (auto key : PREFERED_ORDER) {
        auto it = mapping.find(key);
        if (it != mapping.constEnd()) {
            writeVariant(buffer, it);
        }
    }
    
    for (auto it = mapping.constBegin(); it != mapping.constEnd(); it++) {
        if (!PREFERED_ORDER.contains(it.key())) {
            writeVariant(buffer, it);
        }
    }
    return buffer.data();
}