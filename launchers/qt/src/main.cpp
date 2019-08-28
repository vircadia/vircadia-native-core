#include <iostream>
#include <QGuiApplication>
#include <QString>
#include <QtPlugin>
#include <QResource>
#include <QFileInfo>

#include "LauncherWindow.h"
//Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QtQuick2Plugin);
Q_IMPORT_PLUGIN(QtQuickControls2Plugin);
Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin);
int main(int argc, char *argv[])
{
    QString name { "HQLauncher" };
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName(name);

    QGuiApplication app(argc, argv);

    QString resourceBinaryLocation =  QGuiApplication::applicationDirPath() + "/resources.rcc";
    QResource::registerResource(resourceBinaryLocation);

    LauncherWindow launcherWindow;
    launcherWindow.setFlags(Qt::FramelessWindowHint);
    launcherWindow.setSource(QUrl("qrc:/qml/root.qml"));
    if (launcherWindow.status() == QQuickView::Error) {
        return -1;
    }
    launcherWindow.setResizeMode(QQuickView::SizeRootObjectToView);
    launcherWindow.show();
    return app.exec();
}
