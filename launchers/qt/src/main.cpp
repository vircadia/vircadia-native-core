#include <iostream>
#include <QGuiApplication>
#include <QQuickView>
#include <QString>
#include <QtPlugin>
#include <QResource>
#include <QFileInfo>
//Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QtQuick2Plugin);
Q_IMPORT_PLUGIN(QtQuickControls2Plugin);
Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin);
int main(int argc, char *argv[])
{
    QString name { "QtExamples" };


    std::cout << "Hello world\n";
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName(name);

    QGuiApplication app(argc, argv);

    QString resourceBinaryLocation =  QGuiApplication::applicationDirPath() + "/resources.rcc";
    QResource::registerResource(resourceBinaryLocation);

    QQuickView view;
    view.setFlags(Qt::FramelessWindowHint);
    view.setSource(QUrl("qrc:/qml/root.qml"));
    if (view.status() == QQuickView::Error)
        return -1;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();
    return app.exec();
}
