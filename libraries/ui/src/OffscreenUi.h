//
//  OffscreenUi.h
//  interface/src/entities
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_OffscreenUi_h
#define hifi_OffscreenUi_h

#include <unordered_map>
#include <functional>

#include <QtCore/QVariant>
#include <QtCore/QQueue>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>

#include <gl/OffscreenQmlSurface.h>
#include <DependencyManager.h>

#include "OffscreenQmlElement.h"

class VrMenu;

#define OFFSCREEN_VISIBILITY_PROPERTY "shown"

class OffscreenUi : public OffscreenQmlSurface, public Dependency {
    Q_OBJECT

    friend class VrMenu;
public:
    OffscreenUi();
    virtual void create(QOpenGLContext* context) override;
    void createDesktop(const QUrl& url);
    void show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    void hide(const QString& name);
    void toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    bool shouldSwallowShortcut(QEvent* event);
    bool navigationFocused();
    void setNavigationFocused(bool focused);
    void unfocusWindows();
    void toggleMenu(const QPoint& screenCoordinates);


    // Setting pinned to true will hide all overlay elements on the desktop that don't have a pinned flag
    void setPinned(bool pinned = true);

    void togglePinned();
    void setConstrainToolbarToCenterX(bool constrained);

    bool eventFilter(QObject* originalDestination, QEvent* event) override;
    void addMenuInitializer(std::function<void(VrMenu*)> f);
    QObject* getFlags();
    QQuickItem* getDesktop();
    QQuickItem* getToolWindow();

    enum Icon {
        ICON_NONE = 0,
        ICON_QUESTION,
        ICON_INFORMATION,
        ICON_WARNING,
        ICON_CRITICAL,
        ICON_PLACEMARK
    };

    // Message box compatibility
    Q_INVOKABLE QMessageBox::StandardButton messageBox(Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
    // Must be called from the main thread
    QQuickItem* createMessageBox(Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
    // Must be called from the main thread
    Q_INVOKABLE int waitForMessageBoxResult(QQuickItem* messageBox);

    /// Same design as QMessageBox::critical(), will block, returns result
    static QMessageBox::StandardButton critical(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return critical(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::information(), will block, returns result
    static QMessageBox::StandardButton information(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return information(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::question(), will block, returns result
    static QMessageBox::StandardButton question(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return question(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::warning(), will block, returns result
    static QMessageBox::StandardButton warning(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return warning(title, text, buttons, defaultButton);
    }

    static QMessageBox::StandardButton critical(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton information(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton question(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton warning(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    Q_INVOKABLE QString fileOpenDialog(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);
    Q_INVOKABLE QString fileSaveDialog(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);
    Q_INVOKABLE QString existingDirectoryDialog(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);

    // Compatibility with QFileDialog::getOpenFileName
    static QString getOpenFileName(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);
    // Compatibility with QFileDialog::getSaveFileName
    static QString getSaveFileName(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);
    // Compatibility with QFileDialog::getExistingDirectory
    static QString getExistingDirectory(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);

    Q_INVOKABLE QVariant inputDialog(const Icon icon, const QString& title, const QString& label = QString(), const QVariant& current = QVariant());
    Q_INVOKABLE QVariant customInputDialog(const Icon icon, const QString& title, const QVariantMap& config);
    QQuickItem* createInputDialog(const Icon icon, const QString& title, const QString& label, const QVariant& current);
    QQuickItem* createCustomInputDialog(const Icon icon, const QString& title, const QVariantMap& config);
    QVariant waitForInputDialogResult(QQuickItem* inputDialog);

    // Compatibility with QInputDialog::getText
    static QString getText(void* ignored, const QString & title, const QString & label,
        QLineEdit::EchoMode mode = QLineEdit::Normal, const QString & text = QString(), bool * ok = 0,
        Qt::WindowFlags flags = 0, Qt::InputMethodHints inputMethodHints = Qt::ImhNone) {
        return getText(OffscreenUi::ICON_NONE, title, label, text, ok);
    }
    // Compatibility with QInputDialog::getItem
    static QString getItem(void *ignored, const QString & title, const QString & label, const QStringList & items,
        int current = 0, bool editable = true, bool * ok = 0, Qt::WindowFlags flags = 0,
        Qt::InputMethodHints inputMethodHints = Qt::ImhNone) {
        return getItem(OffscreenUi::ICON_NONE, title, label, items, current, editable, ok);
    }

    static QString getText(const Icon icon, const QString & title, const QString & label, const QString & text = QString(), bool * ok = 0);
    static QString getItem(const Icon icon, const QString & title, const QString & label, const QStringList & items, int current = 0, bool editable = true, bool * ok = 0);
    static QVariant getCustomInfo(const Icon icon, const QString& title, const QVariantMap& config, bool* ok = 0);

    unsigned int getMenuUserDataId() const;

signals:
    void showDesktop();

private:
    QString fileDialog(const QVariantMap& properties);

    QQuickItem* _desktop { nullptr };
    QQuickItem* _toolWindow { nullptr };
    std::unordered_map<int, bool> _pressedKeys;
    VrMenu* _vrMenu { nullptr };
    QQueue<std::function<void(VrMenu*)>> _queuedMenuInitializers; 
};

#endif
