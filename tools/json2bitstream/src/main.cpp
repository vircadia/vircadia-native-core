//
//  main.cpp
//  tools/json2bitstream/src
//
//  Created by Andrzej Kapolka on 6/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <iostream>

#include <QCoreApplication>
#include <QDataStream>
#include <QFile>

#include <AttributeRegistry.h>

using namespace std;

int main (int argc, char** argv) {
    // need the core application for the script engine
    QCoreApplication app(argc, argv);
    
    if (argc < 3) {
        cerr << "Usage: bitstream2json inputfile outputfile [types...]" << endl;
        return 0;
    }
    QFile inputFile(argv[1]);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        cerr << "Failed to open input file: " << inputFile.errorString().toLatin1().constData() << endl;
        return 1;
    }
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(inputFile.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        cerr << "Failed to read input file: " << error.errorString().toLatin1().constData() << endl;
        return 1;
    }
    JSONReader input(document, Bitstream::ALL_GENERICS);
    
    QFile outputFile(argv[2]);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        cerr << "Failed to open output file: " << outputFile.errorString().toLatin1().constData() << endl;
        return 1;
    }
    QDataStream outputData(&outputFile);
    Bitstream output(outputData, Bitstream::FULL_METADATA);
    
    if (argc < 4) {
        // default type is a single QVariant
        QVariant value;
        input >> value;
        output << value;
    
    } else {
        for (int i = 3; i < argc; i++) {
            int type = QMetaType::type(argv[i]);
            if (type == QMetaType::UnknownType) {
                cerr << "Unknown type: " << argv[i] << endl;
                return 1;
            }
            const TypeStreamer* streamer = Bitstream::getTypeStreamer(type);
            if (!streamer) {
                cerr << "Non-streamable type: " << argv[i] << endl;
                return 1;
            }
            QVariant value;
            streamer->putJSONData(input, input.retrieveNextFromContents(), value);
            streamer->write(output, value);
        }
    }
    output.flush();
    
    return 0;
}
