//
//  JSONBreakableMarshal.cpp 
//  libraries/networking/src
//
//  Created by Stephen Birarda on 04/28/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "JSONBreakableMarshal.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

QStringList JSONBreakableMarshal::toStringList(const QJsonValue& jsonValue, const QString& keypath) {
    // setup the string list that will hold our result
    QStringList result;

    // figure out what type of value this is so we know how to act on it
    if (jsonValue.isObject()) {

        QJsonObject jsonObject = jsonValue.toObject();
        
        // enumerate the keys of the QJsonObject
        foreach(const QString& key, jsonObject.keys()) {
            QJsonValue jsonValue = jsonObject[key];
            
            // setup the keypath for this key
            QString valueKeypath = (keypath.isEmpty() ? "" : keypath + ".") + key;
            
            if (jsonValue.isObject() || jsonValue.isArray()) {
                // recursion is required since the value is a QJsonObject or QJsonArray
                result << toStringList(jsonValue, valueKeypath);
            } else {
                // no recursion required, call our toString method to get the string representation
                // append the QStringList resulting from that to our QStringList
                result << toString(jsonValue, valueKeypath);  
            }
        } 
    } else if (jsonValue.isArray()) {
        QJsonArray jsonArray = jsonValue.toArray();
        
        // enumerate the elements in this QJsonArray
        for (int i = 0; i < jsonArray.size(); i++) {
            QJsonValue arrayValue = jsonArray[i];
            
            // setup the keypath for this object with the array index
            QString valueKeypath = (keypath.isEmpty() ? "" : keypath + ".") + "[" + i + "]";
            
            if (arrayValue.isObject() || arrayValue.isArray()) {
                // recursion is required since the value is a QJsonObject or QJsonArray
                // append the QStringList resulting from that to our QStringList
                result << toStringList(arrayValue, valueKeypath);
            } else {
                result << toString(arrayValue, valueKeypath);
            }
        }
    } else {
        // this is a basic value, so set result to whatever toString reports in a QStringList
        result = QStringList() << toString(jsonValue, keypath);
    }

    return result;
}

const QString JSON_NULL_AS_STRING = "null";
const QString JSON_TRUE_AS_STRING = "true";
const QString JSON_FALSE_AS_STRING = "false";
const QString JSON_UNDEFINED_AS_STRING = "undefined";
const QString JSON_UNKNOWN_AS_STRING = "unknown";

QString JSONBreakableMarshal::toString(const QJsonValue& jsonValue, const QString& keypath) {
    // default the value as a string to unknown in case conversion fails
    QString valueAsString = JSON_UNKNOWN_AS_STRING;
    
    // as the QJsonValue what type it is and format its value as a string accordingly
    if (jsonValue.isNull()) {
        valueAsString = JSON_NULL_AS_STRING;
    } else if (jsonValue.isBool()) {
        valueAsString = jsonValue.toBool() ? JSON_TRUE_AS_STRING : JSON_FALSE_AS_STRING;
    } else if (jsonValue.isDouble()) {
        valueAsString = QString::number(jsonValue.toDouble());
    } else if (jsonValue.isString()) {
        valueAsString = QString("\"%1\"").arg(jsonValue.toString());
    } else if (jsonValue.isUndefined()) {
        valueAsString = JSON_UNDEFINED_AS_STRING;
    } else if (jsonValue.isArray() || jsonValue.isObject()) {
        qDebug() << "JSONBreakableMarshal::toString does not handle conversion of a QJsonObject or QJsonArray." 
            << "You should call JSONBreakableMarshal::toStringList instead.";
    } else {
        qDebug() << "Unrecognized QJsonValue - JSONBreakableMarshal cannot convert to string.";
    }

    return QString("%1=%2").arg(keypath, valueAsString);
}

QJsonObject JSONBreakableMarshal::fromStringBuffer(const QByteArray& buffer) {
    QJsonObject result;
    
    // this is a packet of strings sep by null terminators - pull out each string and create a stringlist
    QStringList packetList;
    int currentIndex = 0;
    int currentSeparator = buffer.indexOf('\0');
    
    while (currentIndex < buffer.size() - 1) {
        packetList << QString::fromUtf8(buffer.mid(currentIndex, currentSeparator));

        if (currentSeparator == -1) {
            // no more separators to be found, break out of here so we're not looping for nothing
            break;
        }
        
        // bump the currentIndex up to the last found separator
        currentIndex = currentSeparator + 1;

        // find the index of the next separator, assuming this one wasn't the last one in the packet
        if (currentSeparator < buffer.size() - 1) {
            currentSeparator = buffer.indexOf('\0', currentIndex); 
        }
    }



    return result;
}
