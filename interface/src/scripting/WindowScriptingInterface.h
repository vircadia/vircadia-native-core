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

#ifndef hifi_WindowScriptingInterface_h
#define hifi_WindowScriptingInterface_h

#include <QObject>
#include <QScriptValue>
#include <QString>
#include <QFileDialog>
#include <QComboBox>
#include <QLineEdit>

class WebWindowClass;

class WindowScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(int innerWidth READ getInnerWidth)
    Q_PROPERTY(int innerHeight READ getInnerHeight)
    Q_PROPERTY(int x READ getX)
    Q_PROPERTY(int y READ getY)
public:
    WindowScriptingInterface();
    int getInnerWidth();
    int getInnerHeight();
    int getX();
    int getY();

public slots:
    QScriptValue getCursorPositionX();
    QScriptValue getCursorPositionY();
    void setCursorPosition(int x, int y);
    QScriptValue hasFocus();
    void setFocus();
    void raiseMainWindow();
    QScriptValue alert(const QString& message = "");
    QScriptValue confirm(const QString& message = "");
    QScriptValue prompt(const QString& message = "", const QString& defaultText = "");
    QScriptValue browse(const QString& title = "", const QString& directory = "",  const QString& nameFilter = "");
    QScriptValue save(const QString& title = "", const QString& directory = "",  const QString& nameFilter = "");
    QScriptValue s3Browse(const QString& nameFilter = "");

signals:
    void domainChanged(const QString& domainHostname);
    void inlineButtonClicked(const QString& name);
    void svoImportRequested(const QString& url);
    void domainConnectionRefused(const QString& reason);

private slots:
    QScriptValue showAlert(const QString& message);
    QScriptValue showConfirm(const QString& message);
    QScriptValue showPrompt(const QString& message, const QString& defaultText);
    QScriptValue showBrowse(const QString& title, const QString& directory, const QString& nameFilter,
                            QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen);
    QScriptValue showS3Browse(const QString& nameFilter);

    void chooseDirectory();
    void inlineButtonClicked();

    WebWindowClass* doCreateWebWindow(const QString& title, const QString& url, int width, int height);

private:
    QString jsRegExp2QtRegExp(const QString& string);
};

#endif // hifi_WindowScriptingInterface_h
