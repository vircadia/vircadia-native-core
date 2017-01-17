//
//  Created by Ryan Huffman on 2016-12-14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Trace.h"

#include <chrono>

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QDateTime>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDataStream>
#include <QtCore/QTextStream>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>

#include <BuildInfo.h>

#include "Gzip.h"
#include "PortableHighResolutionClock.h"
#include "shared/GlobalAppProperties.h"

using namespace tracing;

bool tracing::enabled() {
    return DependencyManager::get<Tracer>()->isEnabled();
}

void Tracer::startTracing() {
    std::lock_guard<std::mutex> guard(_eventsMutex);
    if (_enabled) {
        qWarning() << "Tried to enable tracer, but already enabled";
        return;
    }

    _events.clear();
    _enabled = true;
}

void Tracer::stopTracing() {
    std::lock_guard<std::mutex> guard(_eventsMutex);
    if (!_enabled) {
        qWarning() << "Cannot stop tracing, already disabled";
        return;
    }
    _enabled = false;
}

void TraceEvent::writeJson(QTextStream& out) const {
#if 0
    // FIXME QJsonObject serialization is very slow, so we should be using manual JSON serialization
    out << "{";
    out << "\"name\":\"" << name << "\",";
    out << "\"cat\":\"" << category.categoryName() << "\",";
    out << "\"ph\":\"" << QString(type) << "\",";
    out << "\"ts\":\"" << timestamp << "\",";
    out << "\"pid\":\"" << processID << "\",";
    out << "\"tid\":\"" << threadID << "\"";
    //if (!extra.empty()) {
    //    auto it = extra.begin();
    //    for (; it != extra.end(); it++) {
    //        ev[it.key()] = QJsonValue::fromVariant(it.value());
    //    }
    //}
    //if (!args.empty()) {
    //    out << ",\"args\":'
    //}
    out << '}';
#else
    QJsonObject ev {
        { "name", QJsonValue(name) },
        { "cat", category.categoryName() },
        { "ph", QString(type) },
        { "ts", timestamp },
        { "pid", processID },
        { "tid", threadID }
    };
    if (!id.isEmpty()) {
        ev["id"] = id;
    }
    if (!args.empty()) {
        ev["args"] = QJsonObject::fromVariantMap(args);
    }
    if (!extra.empty()) {
        auto it = extra.begin();
        for (; it != extra.end(); it++) {
            ev[it.key()] = QJsonValue::fromVariant(it.value());
        }
    }
    out << QJsonDocument(ev).toJson(QJsonDocument::Compact);
#endif
}

void Tracer::serialize(const QString& originalPath) {

    QString path = originalPath;

    // Filter for specific tokens potentially present in the path:
    auto now = QDateTime::currentDateTime();

    path = path.replace("{DATE}", now.date().toString("yyyyMMdd"));
    path = path.replace("{TIME}", now.time().toString("HHmm"));

    // If the filename is relative, turn it into an absolute path relative to the document directory.
    QFileInfo originalFileInfo(path);
    if (originalFileInfo.isRelative()) {
        QString docsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        path = docsLocation + "/" + path;
        QFileInfo info(path);
        if (!info.absoluteDir().exists()) {
            QString originalRelativePath = originalFileInfo.path();
            QDir(docsLocation).mkpath(originalRelativePath);
        }
    }



    std::list<TraceEvent> currentEvents;
    {
        std::lock_guard<std::mutex> guard(_eventsMutex);
        currentEvents.swap(_events);
    }

    // If the file exists and we can't remove it, fail early
    if (QFileInfo(path).exists() && !QFile::remove(path)) {
        return;
    }

    // If we can't open a temp file for writing, fail early
    QByteArray data;
    {
        QTextStream out(&data);
        out << "[\n";
        bool first = true;
        for (const auto& event : currentEvents) {
            if (first) {
                first = false;
            } else {
                out << ",\n";
            }
            event.writeJson(out);
        }
        out << "\n]";
    }

    if (path.endsWith(".gz")) {
        QByteArray compressed;
        gzip(data, compressed);
        data = compressed;
    } 
    
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return;
        }
        file.write(data);
        file.close();
    }

#if 0
    QByteArray data;
    {

        // "traceEvents":[
        // {"args":{"nv_payload":0},"cat":"hifi.render","name":"render::Scene::processPendingChangesQueue","ph":"B","pid":14796,"tid":21636,"ts":68795933487}

        QJsonArray traceEvents;

        QJsonDocument document {
            QJsonObject {
                { "traceEvents", traceEvents },
                { "otherData", QJsonObject {
                    { "version", QString { "High Fidelity Interface v1.0" } +BuildInfo::VERSION }
                } }
            }
        };
        
        data = document.toJson(QJsonDocument::Compact);
    }
#endif
}

void Tracer::traceEvent(const QLoggingCategory& category, 
        const QString& name, EventType type, 
        qint64 timestamp, qint64 processID, qint64 threadID,
        const QString& id,
        const QVariantMap& args, const QVariantMap& extra) {
    std::lock_guard<std::mutex> guard(_eventsMutex);
    if (!_enabled) {
        return;
    }

    _events.push_back({
        id,
        name,
        type,
        timestamp,
        processID,
        threadID,
        category,
        args,
        extra
    });
}

void Tracer::traceEvent(const QLoggingCategory& category, 
    const QString& name, EventType type, const QString& id, 
    const QVariantMap& args, const QVariantMap& extra) {
    if (!_enabled) {
        return;
    }

    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(p_high_resolution_clock::now().time_since_epoch()).count();
    auto processID = QCoreApplication::applicationPid();
    auto threadID = int64_t(QThread::currentThreadId());

    traceEvent(category, name, type, timestamp, processID, threadID, id, args, extra);
}
