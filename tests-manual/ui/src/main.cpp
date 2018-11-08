#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngine>
#include <QFileSystemModel>

QString getRelativeDir(const QString& relativePath = ".") {
    QDir path(__FILE__);
    path.cdUp();
    path.cdUp();
    auto result = path.absoluteFilePath(relativePath);
    result = path.cleanPath(result) + "/";
    return result;
}

QString getResourcesDir() {
    return getRelativeDir("../../interface/resources");
}

QString getQmlDir() {
    return getRelativeDir("../../interface/resources/qml");
}

QString getScriptsDir() {
    return getRelativeDir("../../scripts");
}

void addImportPath(QQmlApplicationEngine& engine, const QString& relativePath, bool insert = false) {
    QString resolvedPath = getRelativeDir(relativePath);

    qDebug() << "adding import path: " << QDir::toNativeSeparators(resolvedPath);
    engine.addImportPath(resolvedPath);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("Some Company");
    app.setOrganizationDomain("somecompany.com");
    app.setApplicationName("Amazing Application");

    auto scriptsDir = getScriptsDir();
    auto resourcesDir = getResourcesDir();

    QQmlApplicationEngine engine;
    addImportPath(engine, ".");
    addImportPath(engine, "qml");
    addImportPath(engine, resourcesDir);
    addImportPath(engine, resourcesDir + "/qml");
    addImportPath(engine, scriptsDir);
    addImportPath(engine, scriptsDir + "/developer/tests");

    QFontDatabase::addApplicationFont(resourcesDir + "/fonts/FiraSans-Regular.ttf");
    QFontDatabase::addApplicationFont(resourcesDir + "/fonts/FiraSans-SemiBold.ttf");
    QFontDatabase::addApplicationFont(resourcesDir + "/fonts/hifi-glyphs.ttf");

    auto url = getRelativeDir(".") + "qml/ControlsGalleryWindow.qml";

    engine.load(url);
    return app.exec();
}
