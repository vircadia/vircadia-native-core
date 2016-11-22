//
//  WindowScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QClipboard>
#include <QtCore/QDir>
#include <QMessageBox>
#include <QScriptValue>

#include <SettingHandle.h>

#include "Application.h"
#include "DomainHandler.h"
#include "MainWindow.h"
#include "Menu.h"
#include "OffscreenUi.h"

#include "WindowScriptingInterface.h"

static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
static const QString LAST_BROWSE_LOCATION_SETTING = "LastBrowseLocation";


QScriptValue CustomPromptResultToScriptValue(QScriptEngine* engine, const CustomPromptResult& result) {
    if (!result.value.isValid()) {
        return QScriptValue::UndefinedValue;
    }

    Q_ASSERT(result.value.userType() == qMetaTypeId<QVariantMap>());
    return engine->toScriptValue(result.value.toMap());
}

void CustomPromptResultFromScriptValue(const QScriptValue& object, CustomPromptResult& result) {
    result.value = object.toVariant();
}


WindowScriptingInterface::WindowScriptingInterface() {
    const DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
    connect(&domainHandler, &DomainHandler::connectedToDomain, this, &WindowScriptingInterface::domainChanged);
    connect(&domainHandler, &DomainHandler::domainConnectionRefused, this, &WindowScriptingInterface::domainConnectionRefused);

    connect(qApp, &Application::svoImportRequested, [this](const QString& urlString) {
        static const QMetaMethod svoImportRequestedSignal =
            QMetaMethod::fromSignal(&WindowScriptingInterface::svoImportRequested);

        if (isSignalConnected(svoImportRequestedSignal)) {
            QUrl url(urlString);
            emit svoImportRequested(url.url());
        } else {
            OffscreenUi::warning("Import SVO Error", "You need to be running edit.js to import entities.");
        }
    });
}

QScriptValue WindowScriptingInterface::hasFocus() {
    return qApp->hasFocus();
}

void WindowScriptingInterface::setFocus() {
    // It's forbidden to call focus() from another thread.
    qApp->postLambdaEvent([] {
        auto window = qApp->getWindow();
        window->activateWindow();
        window->setFocus();
    });
}

void WindowScriptingInterface::raiseMainWindow() {
    // It's forbidden to call raise() from another thread.
    qApp->postLambdaEvent([] {
        qApp->getWindow()->raise();
    });
}

/// Display an alert box
/// \param const QString& message message to display
/// \return QScriptValue::UndefinedValue
void WindowScriptingInterface::alert(const QString& message) {
    OffscreenUi::warning("", message);
}

/// Display a confirmation box with the options 'Yes' and 'No'
/// \param const QString& message message to display
/// \return QScriptValue `true` if 'Yes' was clicked, `false` otherwise
QScriptValue WindowScriptingInterface::confirm(const QString& message) {
    return QScriptValue((QMessageBox::Yes == OffscreenUi::question("", message, QMessageBox::Yes | QMessageBox::No)));
}

/// Display a prompt with a text box
/// \param const QString& message message to display
/// \param const QString& defaultText default text in the text box
/// \return QScriptValue string text value in text box if the dialog was accepted, `null` otherwise.
QScriptValue WindowScriptingInterface::prompt(const QString& message, const QString& defaultText) {
    bool ok = false;
    QString result = OffscreenUi::getText(nullptr, "", message, QLineEdit::Normal, defaultText, &ok);
    return ok ? QScriptValue(result) : QScriptValue::NullValue;
}

CustomPromptResult WindowScriptingInterface::customPrompt(const QVariant& config) {
    CustomPromptResult result;
    bool ok = false;
    auto configMap = config.toMap();
    result.value = OffscreenUi::getCustomInfo(OffscreenUi::ICON_NONE, "", configMap, &ok);
    return ok ? result : CustomPromptResult();
}

QString fixupPathForMac(const QString& directory) {
    // On OS X `directory` does not work as expected unless a file is included in the path, so we append a bogus
    // filename if the directory is valid.
    QString path = "";
    QFileInfo fileInfo = QFileInfo(directory);
    if (fileInfo.isDir()) {
        fileInfo.setFile(directory, "__HIFI_INVALID_FILE__");
        path = fileInfo.filePath();
    }
    return path;
}

QString WindowScriptingInterface::getPreviousBrowseLocation() const {
    return Setting::Handle<QString>(LAST_BROWSE_LOCATION_SETTING, DESKTOP_LOCATION).get();
}

void WindowScriptingInterface::setPreviousBrowseLocation(const QString& location) {
    Setting::Handle<QVariant>(LAST_BROWSE_LOCATION_SETTING).set(location);
}

/// Display an open file dialog.  If `directory` is an invalid file or directory the browser will start at the current
/// working directory.
/// \param const QString& title title of the window
/// \param const QString& directory directory to start the file browser at
/// \param const QString& nameFilter filter to filter filenames by - see `QFileDialog`
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::browse(const QString& title, const QString& directory, const QString& nameFilter) {
    QString path = directory;
    if (path.isEmpty()) {
        path = getPreviousBrowseLocation();
    }
#ifndef Q_OS_WIN
    path = fixupPathForMac(directory);
#endif
    QString result = OffscreenUi::getOpenFileName(nullptr, title, path, nameFilter);
    if (!result.isEmpty()) {
        setPreviousBrowseLocation(QFileInfo(result).absolutePath());
    }
    return result.isEmpty() ? QScriptValue::NullValue : QScriptValue(result);
}

/// Display a save file dialog.  If `directory` is an invalid file or directory the browser will start at the current
/// working directory.
/// \param const QString& title title of the window
/// \param const QString& directory directory to start the file browser at
/// \param const QString& nameFilter filter to filter filenames by - see `QFileDialog`
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::save(const QString& title, const QString& directory, const QString& nameFilter) {
    QString path = directory;
    if (path.isEmpty()) {
        path = getPreviousBrowseLocation();
    }
#ifndef Q_OS_WIN
    path = fixupPathForMac(directory);
#endif
    QString result = OffscreenUi::getSaveFileName(nullptr, title, path, nameFilter);
    if (!result.isEmpty()) {
        setPreviousBrowseLocation(QFileInfo(result).absolutePath());
    }
    return result.isEmpty() ? QScriptValue::NullValue : QScriptValue(result);
}

void WindowScriptingInterface::showAssetServer(const QString& upload) {
    QMetaObject::invokeMethod(qApp, "showAssetServerWidget", Qt::QueuedConnection, Q_ARG(QString, upload));
}

int WindowScriptingInterface::getInnerWidth() {
    return qApp->getWindow()->geometry().width();
}

int WindowScriptingInterface::getInnerHeight() {
    return qApp->getWindow()->geometry().height();
}

int WindowScriptingInterface::getX() {
    return qApp->getWindow()->x();
}

int WindowScriptingInterface::getY() {
    return qApp->getWindow()->y();
}

void WindowScriptingInterface::copyToClipboard(const QString& text) {
    qDebug() << "Copying";
    QApplication::clipboard()->setText(text);
}

void WindowScriptingInterface::takeSnapshot(bool notify, bool includeAnimated, float aspectRatio) {
    qApp->takeSnapshot(notify, includeAnimated, aspectRatio);
}

void WindowScriptingInterface::shareSnapshot(const QString& path, const QUrl& href) {
    qApp->shareSnapshot(path, href);
}

bool WindowScriptingInterface::isPhysicsEnabled() {
    return qApp->isPhysicsEnabled();
}
