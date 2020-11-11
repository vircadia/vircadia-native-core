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

#include <DependencyManager.h>

#include "ui/OffscreenQmlSurface.h"
#include "OffscreenQmlElement.h"

class VrMenu;

#define OFFSCREEN_VISIBILITY_PROPERTY "shown"

class ModalDialogListener : public QObject {
    Q_OBJECT
    friend class OffscreenUi;

public:
    QQuickItem* getDialogItem() { return _dialog; };

protected:
    ModalDialogListener(QQuickItem* dialog);
    virtual ~ModalDialogListener();
    virtual QVariant waitForResult();

signals:
    void response(const QVariant& value);

protected slots:
    virtual void onDestroyed();

protected:
    QQuickItem* _dialog;
    bool _finished { false };
    QVariant _result;
};

class OffscreenUi : public OffscreenQmlSurface, public Dependency {
    Q_OBJECT

    friend class VrMenu;
public:
    OffscreenUi();
    void createDesktop(const QUrl& url);
    void show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    void hide(const QString& name);
    void hideDesktopWindows();
    bool isVisible(const QString& name);
    void toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    bool shouldSwallowShortcut(QEvent* event);
    bool navigationFocused();
    void setNavigationFocused(bool focused);
    void unfocusWindows();


    // Setting pinned to true will hide all overlay elements on the desktop that don't have a pinned flag
    void setPinned(bool pinned = true);
    
    void togglePinned();
    void setConstrainToolbarToCenterX(bool constrained);

    bool eventFilter(QObject* originalDestination, QEvent* event) override;
    void addMenuInitializer(std::function<void(VrMenu*)> f);
    QObject* getFlags();
    Q_INVOKABLE bool isPointOnDesktopWindow(QVariant point);
    QQuickItem* getDesktop();
    QObject* getRootMenu();
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
    Q_INVOKABLE ModalDialogListener* asyncMessageBox(Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
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
    static ModalDialogListener* asyncCritical(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return asyncCritical(title, text, buttons, defaultButton);
    }
    static ModalDialogListener* asyncInformation(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return asyncInformation(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::question(), will block, returns result
    static QMessageBox::StandardButton question(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return question(title, text, buttons, defaultButton);
    }

    static ModalDialogListener* asyncQuestion(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return asyncQuestion(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::warning(), will block, returns result
    static QMessageBox::StandardButton warning(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return warning(title, text, buttons, defaultButton);
    }
    static ModalDialogListener* asyncWarning(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return asyncWarning(title, text, buttons, defaultButton);
    }

    static QMessageBox::StandardButton critical(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton information(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static ModalDialogListener* asyncCritical(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static ModalDialogListener *asyncInformation(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton question(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static ModalDialogListener* asyncQuestion (const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton warning(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static ModalDialogListener *asyncWarning(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    Q_INVOKABLE QString fileOpenDialog(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    Q_INVOKABLE ModalDialogListener* fileOpenDialogAsync(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());

    Q_INVOKABLE QString fileSaveDialog(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    Q_INVOKABLE ModalDialogListener* fileSaveDialogAsync(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());

    Q_INVOKABLE QString existingDirectoryDialog(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    Q_INVOKABLE ModalDialogListener* existingDirectoryDialogAsync(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());

    Q_INVOKABLE QString assetOpenDialog(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    Q_INVOKABLE ModalDialogListener* assetOpenDialogAsync(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());

    // Compatibility with QFileDialog::getOpenFileName
    static QString getOpenFileName(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    static ModalDialogListener* getOpenFileNameAsync(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());

    // Compatibility with QFileDialog::getSaveFileName
    static QString getSaveFileName(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    static ModalDialogListener* getSaveFileNameAsync(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    // Compatibility with QFileDialog::getExistingDirectory
    static QString getExistingDirectory(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    static ModalDialogListener* getExistingDirectoryAsync(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());

    static QString getOpenAssetName(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());
    static ModalDialogListener* getOpenAssetNameAsync(void* ignored, const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = QFileDialog::Options());

    Q_INVOKABLE QVariant inputDialog(const Icon icon, const QString& title, const QString& label = QString(), const QVariant& current = QVariant());
    Q_INVOKABLE ModalDialogListener* inputDialogAsync(const Icon icon, const QString& title, const QString& label = QString(), const QVariant& current = QVariant());
    Q_INVOKABLE QVariant customInputDialog(const Icon icon, const QString& title, const QVariantMap& config);
    Q_INVOKABLE ModalDialogListener* customInputDialogAsync(const Icon icon, const QString& title, const QVariantMap& config);
    QQuickItem* createInputDialog(const Icon icon, const QString& title, const QString& label, const QVariant& current);
    QQuickItem* createCustomInputDialog(const Icon icon, const QString& title, const QVariantMap& config);
    QVariant waitForInputDialogResult(QQuickItem* inputDialog);

    // Compatibility with QInputDialog::getText
    static QString getText(void* ignored, const QString & title, const QString & label,
        QLineEdit::EchoMode mode = QLineEdit::Normal, const QString & text = QString(), bool * ok = 0,
        Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone) {
        return getText(OffscreenUi::ICON_NONE, title, label, text, ok);
    }
    // Compatibility with QInputDialog::getItem
    static QString getItem(void *ignored, const QString & title, const QString & label, const QStringList & items,
        int current = 0, bool editable = true, bool * ok = 0, Qt::WindowFlags flags = Qt::WindowFlags(),
        Qt::InputMethodHints inputMethodHints = Qt::ImhNone) {
        return getItem(OffscreenUi::ICON_NONE, title, label, items, current, editable, ok);
    }

    // Compatibility with QInputDialog::getText
    static ModalDialogListener* getTextAsync(void* ignored, const QString & title, const QString & label,
        QLineEdit::EchoMode mode = QLineEdit::Normal, const QString & text = QString(), bool * ok = 0,
        Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone) {
        return getTextAsync(OffscreenUi::ICON_NONE, title, label, text);
    }
    // Compatibility with QInputDialog::getItem
    static ModalDialogListener* getItemAsync(void *ignored, const QString & title, const QString & label, const QStringList & items,
        int current = 0, bool editable = true, bool * ok = 0, Qt::WindowFlags flags = Qt::WindowFlags(),
        Qt::InputMethodHints inputMethodHints = Qt::ImhNone) {
        return getItemAsync(OffscreenUi::ICON_NONE, title, label, items, current, editable);
    }

    static QString getText(const Icon icon, const QString & title, const QString & label, const QString & text = QString(), bool * ok = 0);
    static QString getItem(const Icon icon, const QString & title, const QString & label, const QStringList & items, int current = 0, bool editable = true, bool * ok = 0);
    static ModalDialogListener* getTextAsync(const Icon icon, const QString & title, const QString & label, const QString & text = QString());
    static ModalDialogListener* getItemAsync(const Icon icon, const QString & title, const QString & label, const QStringList & items, int current = 0, bool editable = true);

    unsigned int getMenuUserDataId() const;
    QList<QObject *> &getModalDialogListeners();

signals:
    void showDesktop();
//    void response(QMessageBox::StandardButton response);
//    void fileDialogResponse(QString response);
//    void assetDialogResponse(QString response);
//    void inputDialogResponse(QVariant response);
    void desktopReady();
    void keyboardFocusActive();

public slots:
    void removeModalDialog(QObject* modal);

private slots:
    void hoverBeginEvent(const PointerEvent& event);
    void hoverEndEvent(const PointerEvent& event);
    void handlePointerEvent(const PointerEvent& event);

protected:
    void onRootContextCreated(QQmlContext* qmlContext) override;

private:
    QString fileDialog(const QVariantMap& properties);
    ModalDialogListener *fileDialogAsync(const QVariantMap &properties);
    QString assetDialog(const QVariantMap& properties);
    ModalDialogListener* assetDialogAsync(const QVariantMap& properties);

    QQuickItem* _desktop { nullptr };
    QList<QObject*> _modalDialogListeners;
    std::unordered_map<int, bool> _pressedKeys;
    VrMenu* _vrMenu { nullptr };
    QQueue<std::function<void(VrMenu*)>> _queuedMenuInitializers; 
};

#endif
