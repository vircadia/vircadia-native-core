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

#include <QDir>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QScriptValue>
#include <QScrollArea>

#include "Application.h"
#include "DomainHandler.h"
#include "MainWindow.h"
#include "Menu.h"
#include "OffscreenUi.h"
#include "ui/ModelsBrowser.h"
#include "WebWindowClass.h"

#include "WindowScriptingInterface.h"

WindowScriptingInterface::WindowScriptingInterface() {
    const DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
    connect(&domainHandler, &DomainHandler::connectedToDomain, this, &WindowScriptingInterface::domainChanged);
    connect(qApp, &Application::domainConnectionRefused, this, &WindowScriptingInterface::domainConnectionRefused);

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

WebWindowClass* WindowScriptingInterface::doCreateWebWindow(const QString& title, const QString& url, int width, int height) {
    return new WebWindowClass(title, url, width, height);
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

void WindowScriptingInterface::setCursorPosition(int x, int y) {
    QCursor::setPos(x, y);
}

QScriptValue WindowScriptingInterface::getCursorPositionX() {
    return QCursor::pos().x();
}

QScriptValue WindowScriptingInterface::getCursorPositionY() {
    return QCursor::pos().y();
}

QScriptValue WindowScriptingInterface::alert(const QString& message) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showAlert", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QScriptValue, retVal), Q_ARG(const QString&, message));
    return retVal;
}

QScriptValue WindowScriptingInterface::confirm(const QString& message) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showConfirm", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal), Q_ARG(const QString&, message));
    return retVal;
}

QScriptValue WindowScriptingInterface::prompt(const QString& message, const QString& defaultText) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showPrompt", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, message), Q_ARG(const QString&, defaultText));
    return retVal;
}

QScriptValue WindowScriptingInterface::browse(const QString& title, const QString& directory,  const QString& nameFilter) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showBrowse", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, title), Q_ARG(const QString&, directory), Q_ARG(const QString&, nameFilter));
    return retVal;
}

QScriptValue WindowScriptingInterface::save(const QString& title, const QString& directory,  const QString& nameFilter) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showBrowse", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, title), Q_ARG(const QString&, directory), Q_ARG(const QString&, nameFilter),
                              Q_ARG(QFileDialog::AcceptMode, QFileDialog::AcceptSave));
    return retVal;
}

QScriptValue WindowScriptingInterface::s3Browse(const QString& nameFilter) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showS3Browse", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, nameFilter));
    return retVal;
}


/// Display an alert box
/// \param const QString& message message to display
/// \return QScriptValue::UndefinedValue
QScriptValue WindowScriptingInterface::showAlert(const QString& message) {
    OffscreenUi::warning("", message);
    return QScriptValue::UndefinedValue;
}

/// Display a confirmation box with the options 'Yes' and 'No'
/// \param const QString& message message to display
/// \return QScriptValue `true` if 'Yes' was clicked, `false` otherwise
QScriptValue WindowScriptingInterface::showConfirm(const QString& message) {
    bool confirm = false;
    if (QMessageBox::Yes == OffscreenUi::question("", message)) {
        confirm = true;
    }
    return QScriptValue(confirm);
}

void WindowScriptingInterface::chooseDirectory() {
    QPushButton* button = reinterpret_cast<QPushButton*>(sender());

    QString title = button->property("title").toString();
    QString path = button->property("path").toString();
    QRegExp displayAs = button->property("displayAs").toRegExp();
    QRegExp validateAs = button->property("validateAs").toRegExp();
    QString errorMessage = button->property("errorMessage").toString();

    QString directory = QFileDialog::getExistingDirectory(button, title, path);
    if (directory.isEmpty()) {
        return;
    }

    if (!validateAs.exactMatch(directory)) {
        OffscreenUi::warning(NULL, "Invalid Directory", errorMessage);
        return;
    }

    button->setProperty("path", directory);

    displayAs.indexIn(directory);
    QString buttonText = displayAs.cap(1) != "" ? displayAs.cap(1) : ".";
    button->setText(buttonText);
}

void WindowScriptingInterface::inlineButtonClicked() {
    QPushButton* button = reinterpret_cast<QPushButton*>(sender());
    QString name = button->property("name").toString();
    emit inlineButtonClicked(name);
}

QString WindowScriptingInterface::jsRegExp2QtRegExp(const QString& string) {
    // Converts string representation of RegExp from JavaScript format to Qt format.
    return string.mid(1, string.length() - 2)  // No enclosing slashes.
        .replace("\\/", "/");                  // No escaping of forward slash.
}

/// Display a prompt with a text box
/// \param const QString& message message to display
/// \param const QString& defaultText default text in the text box
/// \return QScriptValue string text value in text box if the dialog was accepted, `null` otherwise.
QScriptValue WindowScriptingInterface::showPrompt(const QString& message, const QString& defaultText) {
    bool ok = false;
    QString result = OffscreenUi::getText(nullptr, "", message, QLineEdit::Normal, defaultText, &ok);
    if (!ok) {
        return QScriptValue::NullValue;
    }
    return QScriptValue(result);
}

/// Display a file dialog.  If `directory` is an invalid file or directory the browser will start at the current
/// working directory.
/// \param const QString& title title of the window
/// \param const QString& directory directory to start the file browser at
/// \param const QString& nameFilter filter to filter filenames by - see `QFileDialog`
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::showBrowse(const QString& title, const QString& directory, const QString& nameFilter,
                                                  QFileDialog::AcceptMode acceptMode) {
    // On OS X `directory` does not work as expected unless a file is included in the path, so we append a bogus
    // filename if the directory is valid.
    QString path = "";
    QFileInfo fileInfo = QFileInfo(directory);
    if (fileInfo.isDir()) {
        fileInfo.setFile(directory, "__HIFI_INVALID_FILE__");
        path = fileInfo.filePath();
    }
    
    QFileDialog fileDialog(qApp->getWindow(), title, path, nameFilter);
    fileDialog.setAcceptMode(acceptMode);
    QUrl fileUrl(directory);
    if (acceptMode == QFileDialog::AcceptSave) {
        // TODO -- Setting this breaks the dialog on Linux.  Does it help something on other platforms?
        // fileDialog.setFileMode(QFileDialog::Directory);
        fileDialog.selectFile(fileUrl.fileName());
    }
    if (fileDialog.exec()) {
        return QScriptValue(fileDialog.selectedFiles().first());
    }
    return QScriptValue::NullValue;
}

/// Display a browse window for S3 models
/// \param const QString& nameFilter filter to filter filenames
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::showS3Browse(const QString& nameFilter) {
    ModelsBrowser browser(FSTReader::ENTITY_MODEL);
    if (nameFilter != "") {
        browser.setNameFilter(nameFilter);
    }
    QEventLoop loop;
    connect(&browser, &ModelsBrowser::selected, &loop, &QEventLoop::quit);
    QMetaObject::invokeMethod(&browser, "browse", Qt::QueuedConnection);
    loop.exec();
    
    return browser.getSelectedFile();
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
