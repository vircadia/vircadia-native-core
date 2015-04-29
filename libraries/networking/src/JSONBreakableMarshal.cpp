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

QVariant JSONBreakableMarshal::fromString(const QString& marshalValue) {
    // default the value to null
    QVariant result;
    
    // attempt to match the value with our expected strings
    if (marshalValue == JSON_NULL_AS_STRING) {
        // this is already our default, we don't need to do anything here
    } else if (marshalValue == JSON_TRUE_AS_STRING || marshalValue == JSON_FALSE_AS_STRING) {
        result = QVariant(marshalValue == JSON_TRUE_AS_STRING ? true : false);
    } else if (marshalValue == JSON_UNDEFINED_AS_STRING) {
        result = JSON_UNDEFINED_AS_STRING;  
    } else if (marshalValue == JSON_UNKNOWN_AS_STRING) {
        // we weren't able to marshal this value at the other end, set it as our unknown string"
        result = JSON_UNKNOWN_AS_STRING;
    } else {
        // this might be a double, see if it converts
        bool didConvert = false;
        double convertResult = marshalValue.toDouble(&didConvert);

        if (didConvert) {
            result = convertResult;
        } else {
            // we need to figure out if this is a string
            // use a regex to look for surrounding quotes first
            const QString JSON_STRING_REGEX = "\\A\"([\\w\\W]*)\"\\z";
            QRegExp stringRegex(JSON_STRING_REGEX);
            
            if (stringRegex.indexIn(marshalValue)) {
                // set the result to the string value
                result = stringRegex.cap(1);
            } else {
                // we failed to convert the value to anything, set the result to our unknown value 
                result = JSON_UNKNOWN_AS_STRING; 
            }
        }
    }

    return result;
}

QVariantMap JSONBreakableMarshal::fromStringList(const QStringList& stringList) {
    QVariant result = QVariantMap();

    foreach(const QString& marshalString, stringList) {
        
        // find the equality operator
        int equalityIndex = marshalString.indexOf('=');
        
        // bail on parsing if we didn't find the equality operator
        if (equalityIndex != -1) {
            
            QVariant* currentValue = &result; 

            // pull the key (everything left of the equality sign)
            QString keypath = marshalString.left(equalityIndex);

            // setup for array index checking
            const QString ARRAY_INDEX_REGEX_STRING = "\[\\d+\\]";
            QRegExp arrayRegex(ARRAY_INDEX_REGEX_STRING);
 
            // as long as we have a keypath we need to recurse downwards
            while (!keypath.isEmpty()) {
                int arrayBracketIndex = arrayRegex.indexIn(keypath);

                // is this an array index
                if (arrayBracketIndex == 0) {
                    // we're here because we think the current value should be an array
                    // if it isn't then make one
                    if (!currentValue->canConvert(QMetaType::QVariantList) || currentValue->isNull()) {
                        *currentValue = QVariantList();
                    }

                    QVariantList& currentList = *static_cast<QVariantList*>(currentValue->data());
                    
                    // figure out what index we want to get the QJsonValue& for
                    bool didConvert = false;
                    int arrayIndex = arrayRegex.cap(1).toInt(&didConvert);

                    if (didConvert) {
                        // check if we need to resize the array
                        if (currentList.size() < arrayIndex + 1) {

                            for (int i = currentList.size() - 1; i < arrayIndex + 1; i++) {
                                // add the null QJsonValue at this array index to get the array to the right size
                                currentList.push_back(QJsonValue());                                
                            }
                        }

                        // set our currentValue to the QJsonValue& from the array at this index
                        currentValue = &currentList[arrayIndex];

                        // update the keypath by bumping past the array index
                        keypath = keypath.mid(keypath.indexOf(']') + 1);
                        
                        // check if there is a key after the array index - if so push the keypath forward by a char
                        if (keypath.startsWith(".")) {
                            keypath = keypath.mid(1);
                        }
                    } else {
                        qDebug() << "Failed to convert array index from keypath" << keypath << "to int. Will not add"
                            << "value to resulting QJsonObject.";
                    }

                } else {
                    int keySeparatorIndex = keypath.indexOf('.');
                    
                    // we need to figure out what the key to look at is
                    QString subKey = keypath;

                    if (keySeparatorIndex != -1 || arrayBracketIndex != -1) {
                        int nextBreakIndex = -1;
                        int nextKeypathStartIndex = -1;

                        if (arrayBracketIndex == -1 || (keySeparatorIndex != -1 && keySeparatorIndex < arrayBracketIndex)) {
                            nextBreakIndex = keySeparatorIndex;
                            nextKeypathStartIndex = keySeparatorIndex + 1;  
                        } else if (keySeparatorIndex == -1 || (arrayBracketIndex != -1 
                                   && keySeparatorIndex < arrayBracketIndex)) {
                            nextBreakIndex = arrayBracketIndex;
                            nextKeypathStartIndex = arrayBracketIndex;
                        } else {
                            qDebug() << "Unrecognized key format while trying to parse " << keypath << " - will not add" 
                                << "value to resulting QJsonObject.";
                        }
                        
                        // set the current key from the determined index
                        subKey = keypath.left(nextBreakIndex);
                        
                        // update the keypath being processed
                        keypath = keypath.mid(nextKeypathStartIndex);

                    } else {
                        // update the keypath being processed, since we have no more separators in the keypath, it should
                        // be an empty string
                        keypath = "";
                    }

                    // we're here becuase we know the current value should be an object
                    // if it isn't then make it one
                    
                    if (!currentValue->canConvert(QMetaType::QVariantMap) || currentValue->isNull()) {
                        *currentValue = QVariantMap();
                    }
                    
                    QVariantMap& currentMap = *static_cast<QVariantMap*>(currentValue->data());

                    // is there a QJsonObject for this key yet?
                    // if not then we make it now
                    if (!currentMap.contains(subKey)) {
                        currentMap[subKey] = QVariant();
                    }
                    
                    // change the currentValue to the QJsonValue for this key
                    currentValue = &currentMap[subKey];
                }    
            }

            *currentValue = fromString(marshalString.mid(equalityIndex + 1));
        }
    }

    return result.toMap();
}

QVariantMap JSONBreakableMarshal::fromStringBuffer(const QByteArray& buffer) {
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

    // now that we have a QStringList we use our static method to turn that into a QJsonObject
    return fromStringList(packetList);
}
