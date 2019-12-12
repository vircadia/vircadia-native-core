//
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "treemodel.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // expose model
    TreeModel model;
    engine.rootContext()->setContextProperty("theModel", &model);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            // failure loading main.qml
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
