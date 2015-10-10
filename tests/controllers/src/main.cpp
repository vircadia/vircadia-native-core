//
//  main.cpp
//  tests/gpu-test/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <unordered_map>
#include <memory>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>

#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <QtQuick/QQuickItem>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include <controllers/NewControllerScriptingInterface.h>

const QString& getQmlDir() {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../qml/")) + "/";
        qDebug() << "Qml Path: " << dir;
    }
    return dir;
}

class AppHook : public QQuickItem {
    Q_OBJECT

public:
    AppHook() {
        qDebug() << "Hook Created";
    }
};

using namespace Controllers;

int main(int argc, char** argv) {    
    // Register our component type with QML.
    qmlRegisterType<AppHook>("com.highfidelity.test", 1, 0, "AppHook");
    //qmlRegisterType<NewControllerScriptingInterface>("com.highfidelity.test", 1, 0, "NewControllers");
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine(getQmlDir() + "main.qml");
    engine.rootContext()->setContextProperty("NewControllers", new NewControllerScriptingInterface());
    app.exec();
    return 0;
}



//QQmlEngine engine;
//QQmlComponent *component = new QQmlComponent(&engine);
//
//QObject::connect(&engine, SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));
//
//component->loadUrl(QUrl("main.qml"));
//
//if (!component->isReady()) {
//    qWarning("%s", qPrintable(component->errorString()));
//    return -1;
//}
//
//QObject *topLevel = component->create();
//QQuickWindow *window = qobject_cast<QQuickWindow *>(topLevel);
//
//QSurfaceFormat surfaceFormat = window->requestedFormat();
//window->setFormat(surfaceFormat);
//window->show();
//
//rc = app.exec();
//
//delete component;
//return rc;
//}



#include "main.moc"

