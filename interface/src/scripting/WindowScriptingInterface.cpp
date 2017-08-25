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

#include "WindowScriptingInterface.h"

#include <QClipboard>
#include <QtCore/QDir>
#include <QMessageBox>
#include <QScriptValue>

#include <shared/QtHelpers.h>
#include <SettingHandle.h>

#include <display-plugins/CompositorHelper.h>

#include "Application.h"
#include "DomainHandler.h"
#include "MainWindow.h"
#include "Menu.h"
#include "OffscreenUi.h"

static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
static const QString LAST_BROWSE_LOCATION_SETTING = "LastBrowseLocation";
static const QString LAST_BROWSE_ASSETS_LOCATION_SETTING = "LastBrowseAssetsLocation";


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

    connect(qApp->getWindow(), &MainWindow::windowGeometryChanged, this, &WindowScriptingInterface::geometryChanged);
}

WindowScriptingInterface::~WindowScriptingInterface() {
    QHashIterator<int, QQuickItem*> i(_messageBoxes);
    while (i.hasNext()) {
        i.next();
        auto messageBox = i.value();
        disconnect(messageBox);
        messageBox->setVisible(false);
        messageBox->deleteLater();
    }

    _messageBoxes.clear();
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

QString WindowScriptingInterface::getPreviousBrowseAssetLocation() const {
    QString ASSETS_ROOT_PATH = "/";
    return Setting::Handle<QString>(LAST_BROWSE_ASSETS_LOCATION_SETTING, ASSETS_ROOT_PATH).get();
}

void WindowScriptingInterface::setPreviousBrowseAssetLocation(const QString& location) {
    Setting::Handle<QVariant>(LAST_BROWSE_ASSETS_LOCATION_SETTING).set(location);
}

/// Makes sure that the reticle is visible, use this in blocking forms that require a reticle and
/// might be in same thread as a script that sets the reticle to invisible
void WindowScriptingInterface::ensureReticleVisible() const {
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    if (!compositorHelper->getReticleVisible()) {
        compositorHelper->setReticleVisible(true);
    }
}

/// Display a "browse to directory" dialog.  If `directory` is an invalid file or directory the browser will start at the current
/// working directory.
/// \param const QString& title title of the window
/// \param const QString& directory directory to start the file browser at
/// \param const QString& nameFilter filter to filter filenames by - see `QFileDialog`
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::browseDir(const QString& title, const QString& directory) {
    ensureReticleVisible();
    QString path = directory;
    if (path.isEmpty()) {
        path = getPreviousBrowseLocation();
    }
#ifndef Q_OS_WIN
    path = fixupPathForMac(directory);
#endif
    QString result = OffscreenUi::getExistingDirectory(nullptr, title, path);
    if (!result.isEmpty()) {
        setPreviousBrowseLocation(QFileInfo(result).absolutePath());
    }
    return result.isEmpty() ? QScriptValue::NullValue : QScriptValue(result);
}

/// Display an open file dialog.  If `directory` is an invalid file or directory the browser will start at the current
/// working directory.
/// \param const QString& title title of the window
/// \param const QString& directory directory to start the file browser at
/// \param const QString& nameFilter filter to filter filenames by - see `QFileDialog`
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::browse(const QString& title, const QString& directory, const QString& nameFilter) {
    ensureReticleVisible();
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
    ensureReticleVisible();
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

/// Display a select asset dialog that lets the user select an asset from the Asset Server.  If `directory` is an invalid 
/// directory the browser will start at the root directory.
/// \param const QString& title title of the window
/// \param const QString& directory directory to start the asset browser at
/// \param const QString& nameFilter filter to filter asset names by - see `QFileDialog`
/// \return QScriptValue asset path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::browseAssets(const QString& title, const QString& directory, const QString& nameFilter) {
    ensureReticleVisible();
    QString path = directory;
    if (path.isEmpty()) {
        path = getPreviousBrowseAssetLocation();
    }
    if (path.left(1) != "/") {
        path = "/" + path;
    }
    if (path.right(1) != "/") {
        path = path + "/";
    }
    QString result = OffscreenUi::getOpenAssetName(nullptr, title, path, nameFilter);
    if (!result.isEmpty()) {
        setPreviousBrowseAssetLocation(QFileInfo(result).absolutePath());
    }
    return result.isEmpty() ? QScriptValue::NullValue : QScriptValue(result);
}

void WindowScriptingInterface::showAssetServer(const QString& upload) {
    QMetaObject::invokeMethod(qApp, "showAssetServerWidget", Qt::QueuedConnection, Q_ARG(QString, upload));
}

QString WindowScriptingInterface::checkVersion() {
    return QCoreApplication::applicationVersion();
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


bool WindowScriptingInterface::setDisplayTexture(const QString& name) {
    return  qApp->getActiveDisplayPlugin()->setDisplayTexture(name);   // Plugins that don't know how, answer false.
}

void WindowScriptingInterface::takeSnapshot(bool notify, bool includeAnimated, float aspectRatio) {
    qApp->takeSnapshot(notify, includeAnimated, aspectRatio);
}

void WindowScriptingInterface::takeSecondaryCameraSnapshot() {
    qApp->takeSecondaryCameraSnapshot();
}

void WindowScriptingInterface::shareSnapshot(const QString& path, const QUrl& href) {
    qApp->shareSnapshot(path, href);
}

void WindowScriptingInterface::makeConnection(bool success, const QString& userNameOrError) {
    if (success) {
        emit connectionAdded(userNameOrError);
    } else {
        emit connectionError(userNameOrError);
    }
}

void WindowScriptingInterface::displayAnnouncement(const QString& message) {
    emit announcement(message);
}

bool WindowScriptingInterface::isPhysicsEnabled() {
    return qApp->isPhysicsEnabled();
}

int WindowScriptingInterface::openMessageBox(QString title, QString text, int buttons, int defaultButton) {
    if (QThread::currentThread() != thread()) {
        int result;
        BLOCKING_INVOKE_METHOD(this, "openMessageBox",
            Q_RETURN_ARG(int, result),
            Q_ARG(QString, title),
            Q_ARG(QString, text),
            Q_ARG(int, buttons),
            Q_ARG(int, defaultButton));
        return result;
    }

    return createMessageBox(title, text, buttons, defaultButton);
}

int WindowScriptingInterface::createMessageBox(QString title, QString text, int buttons, int defaultButton) {
    auto messageBox = DependencyManager::get<OffscreenUi>()->createMessageBox(OffscreenUi::ICON_INFORMATION, title, text,
        static_cast<QFlags<QMessageBox::StandardButton>>(buttons), static_cast<QMessageBox::StandardButton>(defaultButton));
    connect(messageBox, SIGNAL(selected(int)), this, SLOT(onMessageBoxSelected(int)));

    _lastMessageBoxID += 1;
    _messageBoxes.insert(_lastMessageBoxID, messageBox);

    return _lastMessageBoxID;
}

void WindowScriptingInterface::updateMessageBox(int id, QString title, QString text, int buttons, int defaultButton) {
    auto messageBox = _messageBoxes.value(id);
    if (messageBox) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "updateMessageBox",
                Q_ARG(int, id),
                Q_ARG(QString, title),
                Q_ARG(QString, text),
                Q_ARG(int, buttons),
                Q_ARG(int, defaultButton));
            return;
        }

        messageBox->setProperty("title", title);
        messageBox->setProperty("text", text);
        messageBox->setProperty("buttons", buttons);
        messageBox->setProperty("defaultButton", defaultButton);
    }
}

void WindowScriptingInterface::closeMessageBox(int id) {
    auto messageBox = _messageBoxes.value(id);
    if (messageBox) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "closeMessageBox",
                Q_ARG(int, id));
            return;
        }

        disconnect(messageBox);
        messageBox->setVisible(false);
        messageBox->deleteLater();
        _messageBoxes.remove(id);
    }
}

void WindowScriptingInterface::onMessageBoxSelected(int button) {
    auto messageBox = qobject_cast<QQuickItem*>(sender());
    auto keys = _messageBoxes.keys(messageBox);
    if (keys.length() > 0) {
        auto id = keys[0];  // Should be just one message box.
        emit messageBoxClosed(id, button);
        disconnect(messageBox);
        messageBox->setVisible(false);
        messageBox->deleteLater();
        _messageBoxes.remove(id);
    }
}
