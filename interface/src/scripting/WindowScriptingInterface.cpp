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

#include <QMessageBox>
#include <QInputDialog>

#include "Application.h"
#include "Menu.h"

#include "WindowScriptingInterface.h"

WindowScriptingInterface* WindowScriptingInterface::getInstance() {
    static WindowScriptingInterface sharedInstance;
    return &sharedInstance;
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

/// Display an alert box
/// \param const QString& message message to display
/// \return QScriptValue::UndefinedValue
QScriptValue WindowScriptingInterface::showAlert(const QString& message) {
    QMessageBox::warning(Application::getInstance()->getWindow(), "", message);
    return QScriptValue::UndefinedValue;
}

/// Display a confirmation box with the options 'Yes' and 'No'
/// \param const QString& message message to display
/// \return QScriptValue `true` if 'Yes' was clicked, `false` otherwise
QScriptValue WindowScriptingInterface::showConfirm(const QString& message) {
    QMessageBox::StandardButton response = QMessageBox::question(Application::getInstance()->getWindow(), "", message);
    return QScriptValue(response == QMessageBox::Yes);
}

/// Display a prompt with a text box
/// \param const QString& message message to display
/// \param const QString& defaultText default text in the text box
/// \return QScriptValue string text value in text box if the dialog was accepted, `null` otherwise.
QScriptValue WindowScriptingInterface::showPrompt(const QString& message, const QString& defaultText) {
    QInputDialog promptDialog(Application::getInstance()->getWindow());
    promptDialog.setWindowTitle("");
    promptDialog.setLabelText(message);
    promptDialog.setTextValue(defaultText);

    if (promptDialog.exec() == QDialog::Accepted) {
        return QScriptValue(promptDialog.textValue());
    }

    return QScriptValue::NullValue;
}

int WindowScriptingInterface::getInnerWidth() {
    return Application::getInstance()->getWindow()->geometry().width();
}

int WindowScriptingInterface::getInnerHeight() {
    return Application::getInstance()->getWindow()->geometry().height();
}
