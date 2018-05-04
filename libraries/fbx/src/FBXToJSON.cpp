//
//  FBXToJSON.cpp
//  libraries/fbx/src
//
//  Created by Simon Walton on 5/4/2013.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FBXToJSON.h"
#include "FBX.h"

template<typename T>
inline FBXToJSON& FBXToJSON::operator<<(QVector<T>& arrayProp) {
    *this << "[";
    for (auto& prop : arrayProp) {
        *(std::ostringstream*)this << prop << ", ";
    }
    *this << "] ";
    if (arrayProp.size() > 4) {
        *this << "# " << arrayProp.size() << " items";
    }
    *this << '\n';

    return *this;
}

FBXToJSON& FBXToJSON::operator<<(const FBXNode& fbxNode) {
    using std::string;

    string nodeName(fbxNode.name);
    if (nodeName.empty()) nodeName = "nodename";

    *this << string(_indentLevel * 4, ' ') << '"' << nodeName << "\":    {\n";
    ++_indentLevel;
    int p = 0;
    for (auto& prop : fbxNode.properties) {
        *this << string(_indentLevel * 4, ' ') << "\"p" << p++ << "\":   ";
        switch (prop.userType()) {
        case QMetaType::Short:
        case QMetaType::Bool:
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::Double:
        case QMetaType::Float:
            *this << prop.toString().toStdString();
            break;

        case QMetaType::QString:
        case QMetaType::QByteArray:
            *this << '"' << prop.toString().toStdString() << '"';
            break;
           
        default:
            if (prop.canConvert<QVector<float>>()) {
                *this << prop.value<QVector<float>>();
            } else if (prop.canConvert<QVector<double>>()) {
                *this << prop.value<QVector<double>>();
            } else if (prop.canConvert<QVector<bool>>()) {
                *this << prop.value<QVector<bool>>();
            } else if (prop.canConvert<QVector<qint32>>()) {
                *this << prop.value<QVector<qint32>>();
            } else if (prop.canConvert<QVector<qint64>>()) {
                *this << prop.value<QVector<qint64>>();
            } else {
                *this << "<unimplemented value>";
            }
            break;
        }
        *this << ",\n";
    }

    for (auto child : fbxNode.children) {
        *this << child;
    }

    *this << string(_indentLevel * 4, ' ') << "},\n";
    --_indentLevel;
    return *this;
}
