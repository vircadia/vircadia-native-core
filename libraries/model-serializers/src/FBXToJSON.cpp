//
//  FBXToJSON.cpp
//  libraries/model-serializers/src
//
//  Created by Simon Walton on 5/4/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FBXToJSON.h"
#include "FBX.h"

using std::string;

template<typename T>
inline FBXToJSON& FBXToJSON::operator<<(const QVector<T>& arrayProp) {
    *this << "[";
    char comma = ' ';
    for (auto& prop : arrayProp) {
        *(std::ostringstream*)this << comma << prop;
        comma = ',';
    }
    *this << "] ";

    if (arrayProp.size() > 4) {
        *this << "// " << arrayProp.size() << " items";
    }
    *this << '\n';

    return *this;
}

FBXToJSON& FBXToJSON::operator<<(const FBXNode& fbxNode) {
    string nodeName(fbxNode.name);
    if (nodeName.empty()) {
        nodeName = "node";
    } else {
        nodeName = stringEscape(nodeName);
    }

    *this << string(_indentLevel * 4, ' ') << '"' << nodeName << "\":    {\n";

    ++_indentLevel;
    int p = 0;
    const char* eol = "";
    for (auto& prop : fbxNode.properties) {
        *this << eol << string(_indentLevel * 4, ' ') << "\"p" << p++ << "\":   ";
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
            *this << '"' << stringEscape(prop.toByteArray().toStdString()) << '"';
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
        eol = ",\n";
    }

    for (auto& child : fbxNode.children) {
        *this << eol;
        *this << child;
        eol = ",\n";
    }

    *this << "\n" << string(_indentLevel * 4, ' ') << "}";
    --_indentLevel;
    return *this;
}

string FBXToJSON::stringEscape(const string& in) {
    string out;
    out.reserve(in.length());

    for (unsigned char inChar: in) {
        if (inChar == '"') {
            out.append(R"(\")");
        }
        else if (inChar == '\\') {
            out.append(R"(\\)");
        }
        else if (inChar < 0x20 || inChar == 0x7f) {
            char h[5];
            sprintf(h, "\\x%02x", inChar);
            out.append(h);
        }
        else out.append(1, inChar);
    }
    return out;
}
