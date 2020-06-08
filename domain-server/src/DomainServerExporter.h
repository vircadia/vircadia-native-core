//
//  DomainServerExporter.h
//  domain-server/src
//
//  Created by Dale Glass on 3 Apr 2020.
//  Copyright 2020 Dale Glass
//
//  Prometheus exporter
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef DOMAINSERVEREXPORTER_H
#define DOMAINSERVEREXPORTER_H

#include <QObject>
#include "HTTPManager.h"
#include "Node.h"
#include <QTextStream>
#include <QJsonObject>
#include <QRegularExpression>
#include <QHash>



/**
 * @brief Prometheus exporter for domain stats
 *
 * This class exportors the statistics that can be seen on the domain's page in
 * a format that can be parsed by Prometheus. This is useful for troubleshooting,
 * monitoring performance, and making pretty graphs.
 */
class DomainServerExporter : public HTTPRequestHandler
{
public:
    typedef enum {
        Untyped,     /* Works the same as Gauge, with the difference of signalling that the actual type is unknown */
        Counter,     /* Value only goes up. Eg, number of packets received */
        Gauge,       /* Current numerical value that can go up or down. Current temperature, memory usage, etc */
        Histogram,   /* Samples sorted in buckets gathered over time */
        Summary
    } MetricType;

    DomainServerExporter();
    ~DomainServerExporter() = default;
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false) override;

private:
    QString escapeName(const QString &name);
    void generateMetricsForNode(QTextStream& stream, const SharedNodePointer& node);
    void generateMetricsFromJson(QTextStream& stream, QString originalPath, QString path, QHash<QString, QString> labels, const QJsonObject& obj);
};

#endif // DOMAINSERVEREXPORTER_H
