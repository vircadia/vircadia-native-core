//
//  SettingHelpers.cpp
//  libraries/shared/src
//
//  Created by Clement on 9/13/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SettingHelpers.h"

#include <QDataStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPoint>
#include <QRect>
#include <QSettings>
#include <QSize>
#include <QStringList>


QSettings::SettingsMap jsonDocumentToVariantMap(const QJsonDocument& document);
QJsonDocument variantMapToJsonDocument(const QSettings::SettingsMap& map);

bool readJSONFile(QIODevice& device, QSettings::SettingsMap& map) {
    QJsonParseError jsonParseError;

    auto bytesRead = device.readAll();
    auto document = QJsonDocument::fromJson(bytesRead, &jsonParseError);

    if (jsonParseError.error != QJsonParseError::NoError) {
        qDebug() << "Error parsing QSettings file:" << jsonParseError.errorString();
        return false;
    }

    map = jsonDocumentToVariantMap(document);

    return true;
}

bool writeJSONFile(QIODevice& device, const QSettings::SettingsMap& map) {
    auto document = variantMapToJsonDocument(map);
    auto jsonByteArray = document.toJson(QJsonDocument::Indented);
    auto bytesWritten = device.write(jsonByteArray);
    return bytesWritten == jsonByteArray.size();
}

void loadOldINIFile(QSettings& settings) {
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QSettings iniSettings;
    if (!iniSettings.allKeys().isEmpty()) {
        qDebug() << "No data in json settings file, trying to load old ini settings file.";

        for (auto key : iniSettings.allKeys()) {
            auto variant = iniSettings.value(key);

            if (variant.type() == QVariant::String) {
                auto string = variant.toString();
                if (string == "true") {
                    variant = true;
                } else if (string == "false") {
                    variant = false;
                } else {
                    bool ok;
                    double value = string.toDouble(&ok);
                    if (ok) {
                        variant = value;
                    }
                }
            }
            settings.setValue(key, variant);
        }

        qDebug() << "Loaded" << settings.allKeys().size() << "keys from ini settings file.";
    }

    QSettings::setDefaultFormat(JSON_FORMAT);
}

QStringList splitArgs(const QString& string, int idx) {
    int length = string.length();
    Q_ASSERT(length > 0);
    Q_ASSERT(string.at(idx) == QLatin1Char('('));
    Q_ASSERT(string.at(length - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < length; ++idx) {
        QChar c = string.at(idx);
        if (c == QLatin1Char(')')) {
            Q_ASSERT(idx == length - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}

QJsonDocument variantMapToJsonDocument(const QSettings::SettingsMap& map) {
    QJsonObject object;
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        auto& key = it.key();
        auto& variant = it.value();
        auto variantType = variant.type();

        // Switch some types so they are readable/modifiable in the json file
        if (variantType == QVariant(1.0f).type()) { // float
            variantType = QVariant::Double;
        }
        if (variantType == QVariant((quint16)0).type()) { // uint16
            variantType = QVariant::UInt;
        }
        if (variantType == QVariant::Url) { // QUrl
            variantType = QVariant::String;
        }

        switch (variantType) {
            case QVariant::Map:
            case QVariant::List:
            case QVariant::Hash: {
                qCritical() << "Unsupported variant type" << variant.typeName();
                Q_ASSERT(false);
                break;
            }

            case QVariant::Invalid:
                object.insert(key, QJsonValue());
                break;
            case QVariant::LongLong:
            case QVariant::ULongLong:
            case QVariant::Int:
            case QVariant::UInt:
            case QVariant::Bool:
            case QVariant::Double:
                object.insert(key, QJsonValue::fromVariant(variant));
                break;

            case QVariant::String: {
                QString result = variant.toString();
                if (result.startsWith(QLatin1Char('@'))) {
                    result.prepend(QLatin1Char('@'));
                }
                object.insert(key, result);
                break;
            }

            case QVariant::ByteArray: {
                QByteArray a = variant.toByteArray();
                QString result = QLatin1String("@ByteArray(");
                result += QString::fromLatin1(a.constData(), a.size());
                result += QLatin1Char(')');
                object.insert(key, result);
                break;
            }
            case QVariant::Rect: {
                QRect r = qvariant_cast<QRect>(variant);
                QString result = QLatin1String("@Rect(");
                result += QString::number(r.x());
                result += QLatin1Char(' ');
                result += QString::number(r.y());
                result += QLatin1Char(' ');
                result += QString::number(r.width());
                result += QLatin1Char(' ');
                result += QString::number(r.height());
                result += QLatin1Char(')');
                object.insert(key, result);
                break;
            }
            case QVariant::Size: {
                QSize s = qvariant_cast<QSize>(variant);
                QString result = QLatin1String("@Size(");
                result += QString::number(s.width());
                result += QLatin1Char(' ');
                result += QString::number(s.height());
                result += QLatin1Char(')');
                object.insert(key, result);
                break;
            }
            case QVariant::Point: {
                QPoint p = qvariant_cast<QPoint>(variant);
                QString result = QLatin1String("@Point(");
                result += QString::number(p.x());
                result += QLatin1Char(' ');
                result += QString::number(p.y());
                result += QLatin1Char(')');
                object.insert(key, result);
                break;
            }

            default: {
                QByteArray array;
                {
                    QDataStream stream(&array, QIODevice::WriteOnly);
                    stream.setVersion(QDataStream::Qt_4_0);
                    stream << variant;
                }

                QString result = QLatin1String("@Variant(");
                result += QString::fromLatin1(array.constData(), array.size());
                result += QLatin1Char(')');
                object.insert(key, result);
                break;
            }
        }
    }

    return QJsonDocument(object);
}


QSettings::SettingsMap jsonDocumentToVariantMap(const QJsonDocument& document) {
    if (!document.isObject()) {
        qWarning() << "Settings file does not contain a JSON object";
        return QSettings::SettingsMap();
    }
    auto object = document.object();
    QSettings::SettingsMap map;

    for (auto it = object.begin(); it != object.end(); ++it) {

        QVariant result;

        if (!it->isString()) {
            result = it->toVariant();
        } else {
            auto string = it->toString();

            if (string.startsWith(QLatin1String("@@"))) { // Standard string starting with '@'
                result = QVariant(string.mid(1));

            } else if (string.startsWith(QLatin1Char('@'))) { // Custom type to string

                if (string.endsWith(QLatin1Char(')'))) {

                    if (string.startsWith(QLatin1String("@ByteArray("))) {
                        result = QVariant(string.toLatin1().mid(11, string.size() - 12));

                    } else if (string.startsWith(QLatin1String("@Variant("))) {
                        QByteArray a(string.toLatin1().mid(9));
                        QDataStream stream(&a, QIODevice::ReadOnly);
                        stream.setVersion(QDataStream::Qt_4_0);
                        stream >> result;

                    } else if (string.startsWith(QLatin1String("@Rect("))) {
                        QStringList args = splitArgs(string, 5);
                        if (args.size() == 4) {
                            result = QRect(args[0].toInt(), args[1].toInt(),
                                           args[2].toInt(), args[3].toInt());
                        }

                    } else if (string.startsWith(QLatin1String("@Size("))) {
                        QStringList args = splitArgs(string, 5);
                        if (args.size() == 2) {
                            result = QSize(args[0].toInt(), args[1].toInt());
                        }

                    } else if (string.startsWith(QLatin1String("@Point("))) {
                        QStringList args = splitArgs(string, 6);
                        if (args.size() == 2) {
                            result = QPoint(args[0].toInt(), args[1].toInt());
                        }
                    }
                }
            } else { // Standard string
                result = string;
            }
        }

        map.insert(it.key(), result);
    }

    return map;
}
