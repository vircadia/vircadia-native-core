#include <iostream>
#include <QGuiApplication>
#include <QQuickView>
#include <QString>
#include <QtPlugin>
/*Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QtQuick2Plugin);
Q_IMPORT_PLUGIN(QtQuickControls2Plugin);
Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin);*/
int main(int argc, char *argv[])
{
    QString name { "QtExamples" };
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName(name);

    QGuiApplication app(argc, argv);

    QQuickView view;
    view.setFlags(Qt::FramelessWindowHint);
    view.setSource(QUrl("/Users/danteruiz/github/test/qml/root.qml"));
    if (view.status() == QQuickView::Error)
        return -1;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();
    return app.exec();
}
