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
#include <mutex>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>

#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <QtQuick/QQuickItem>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include <controllers/NewControllerScriptingInterface.h>
#include <DependencyManager.h>
#include <plugins/PluginManager.h>
#include <input-plugins/InputPlugin.h>
#include <input-plugins/KeyboardMouseDevice.h>
#include <input-plugins/UserInputMapper.h>

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


QString sanatizeName(const QString& name) {
    QString cleanName{ name };
    cleanName.remove(QRegularExpression{ "[\\(\\)\\.\\s]" });
    return cleanName;
}

const QVariantMap& getInputMap() {
    static std::once_flag once;
    static QVariantMap map;
    std::call_once(once, [&] {
        {
            QVariantMap hardwareMap;
            // Controller.Hardware.*
            auto devices = DependencyManager::get<UserInputMapper>()->getDevices();
            for (const auto& deviceMapping : devices) {
                auto device = deviceMapping.second.get();
                auto deviceName = sanatizeName(device->getName());
                auto deviceInputs = device->getAvailabeInputs();
                QVariantMap deviceMap;
                for (const auto& inputMapping : deviceInputs) {
                    auto input = inputMapping.first;
                    auto inputName = sanatizeName(inputMapping.second);
                    deviceMap.insert(inputName, input.getID());
                }
                hardwareMap.insert(deviceName, deviceMap);
            }
            map.insert("Hardware", hardwareMap);
        }

        // Controller.Actions.*
        {
            QVariantMap actionMap;
            auto actionNames = DependencyManager::get<UserInputMapper>()->getActionNames();
            int actionNumber = 0;
            for (const auto& actionName : actionNames) {
                actionMap.insert(sanatizeName(actionName), actionNumber++);
            }
            map.insert("Actions", actionMap);
        }
    });
    return map;
}

int main(int argc, char** argv) {
    DependencyManager::set<UserInputMapper>();
    PluginManager::getInstance()->getInputPlugins();

    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        QString name = inputPlugin->getName();
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        if (name == KeyboardMouseDevice::NAME) {
            auto keyboardMouseDevice = static_cast<KeyboardMouseDevice*>(inputPlugin.data()); // TODO: this seems super hacky
            keyboardMouseDevice->registerToUserInputMapper(*userInputMapper);
            keyboardMouseDevice->assignDefaultInputMapping(*userInputMapper);
        }
    }

    // Register our component type with QML.
    qmlRegisterType<AppHook>("com.highfidelity.test", 1, 0, "AppHook");
    //qmlRegisterType<NewControllerScriptingInterface>("com.highfidelity.test", 1, 0, "NewControllers");
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("NewControllers", new NewControllerScriptingInterface());
    engine.rootContext()->setContextProperty("ControllerIds", getInputMap());
    engine.load(getQmlDir() + "main.qml");
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

