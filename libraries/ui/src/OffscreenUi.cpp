//
//  OffscreenUi.cpp
//  interface/src/render-utils
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OffscreenUi.h"

#include <QtCore/QVariant>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickWindow>
#include <QtQml/QtQml>

#include <gl/GLHelpers.h>

#include <AbstractUriHandler.h>
#include <AccountManager.h>

#include "FileDialogHelper.h"
#include "VrMenu.h"


// Needs to match the constants in resources/qml/Global.js
class OffscreenFlags : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool navigationFocused READ isNavigationFocused WRITE setNavigationFocused NOTIFY navigationFocusedChanged)

    // Allow scripts that are doing their own navigation support to disable navigation focus (i.e. handControllerPointer.js)
    Q_PROPERTY(bool navigationFocusDisabled READ isNavigationFocusDisabled WRITE setNavigationFocusDisabled NOTIFY navigationFocusDisabledChanged)

public:

    OffscreenFlags(QObject* parent = nullptr) : QObject(parent) {}
    bool isNavigationFocused() const { return _navigationFocused; }
    void setNavigationFocused(bool focused) {
        if (_navigationFocused != focused) {
            _navigationFocused = focused;
            emit navigationFocusedChanged();
        }
    }

    bool isNavigationFocusDisabled() const { return _navigationFocusDisabled; }
    void setNavigationFocusDisabled(bool disabled) {
        if (_navigationFocusDisabled != disabled) {
            _navigationFocusDisabled = disabled;
            emit navigationFocusDisabledChanged();
        }
    }
    
signals:
    void navigationFocusedChanged();
    void navigationFocusDisabledChanged();

private:
    bool _navigationFocused { false };
    bool _navigationFocusDisabled{ false };
};

QString fixupHifiUrl(const QString& urlString) {
	static const QString ACCESS_TOKEN_PARAMETER = "access_token";
	static const QString ALLOWED_HOST = "metaverse.highfidelity.com";
    QUrl url(urlString);
	QUrlQuery query(url);
	if (url.host() == ALLOWED_HOST && query.allQueryItemValues(ACCESS_TOKEN_PARAMETER).empty()) {
	    auto accountManager = DependencyManager::get<AccountManager>();
	    query.addQueryItem(ACCESS_TOKEN_PARAMETER, accountManager->getAccountInfo().getAccessToken().token);
	    url.setQuery(query.query());
	    return url.toString();
	}
    return urlString;
}

class UrlHandler : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE bool canHandleUrl(const QString& url) {
        static auto handler = dynamic_cast<AbstractUriHandler*>(qApp);
        return handler->canAcceptURL(url);
    }

    Q_INVOKABLE bool handleUrl(const QString& url) {
        static auto handler = dynamic_cast<AbstractUriHandler*>(qApp);
        return handler->acceptURL(url);
    }
    
    // FIXME hack for authentication, remove when we migrate to Qt 5.6
    Q_INVOKABLE QString fixupUrl(const QString& originalUrl) {
        return fixupHifiUrl(originalUrl);
    }
};

static OffscreenFlags* offscreenFlags { nullptr };

// This hack allows the QML UI to work with keys that are also bound as 
// shortcuts at the application level.  However, it seems as though the 
// bound actions are still getting triggered.  At least for backspace.  
// Not sure why.
//
// However, the problem may go away once we switch to the new menu system,
// so I think it's OK for the time being.
bool OffscreenUi::shouldSwallowShortcut(QEvent* event) {
    Q_ASSERT(event->type() == QEvent::ShortcutOverride);
    QObject* focusObject = getWindow()->focusObject();
    if (focusObject != getWindow() && focusObject != getRootItem()) {
        //qDebug() << "Swallowed shortcut " << static_cast<QKeyEvent*>(event)->key();
        event->accept();
        return true;
    }
    return false;
}

OffscreenUi::OffscreenUi() {
}

QObject* OffscreenUi::getFlags() {
    return offscreenFlags;
}

void OffscreenUi::create(QOpenGLContext* context) {
    OffscreenQmlSurface::create(context);
    auto rootContext = getRootContext();

    rootContext->setContextProperty("OffscreenUi", this);
    rootContext->setContextProperty("offscreenFlags", offscreenFlags = new OffscreenFlags());
    rootContext->setContextProperty("urlHandler", new UrlHandler());
    rootContext->setContextProperty("fileDialogHelper", new FileDialogHelper());
}

void OffscreenUi::show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    emit showDesktop();
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = getRootItem()->findChild<QQuickItem*>(name);
    }

    if (item) {
        QQmlProperty(item, OFFSCREEN_VISIBILITY_PROPERTY).write(true);
    }
}

void OffscreenUi::toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    if (!item) {
        show(url, name, f);
        return;
    }

    // Already loaded, so just flip the bit
    QQmlProperty shownProperty(item, OFFSCREEN_VISIBILITY_PROPERTY);
    shownProperty.write(!shownProperty.read().toBool());
}

void OffscreenUi::hide(const QString& name) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    if (item) {
        QQmlProperty(item, OFFSCREEN_VISIBILITY_PROPERTY).write(false);
    }
}

class ModalDialogListener : public QObject {
    Q_OBJECT
    friend class OffscreenUi;

protected:
    ModalDialogListener(QQuickItem* dialog) : _dialog(dialog) {
        if (!dialog) {
            _finished = true;
            return;
        }
        connect(_dialog, SIGNAL(destroyed()), this, SLOT(onDestroyed()));
    }

    ~ModalDialogListener() {
        if (_dialog) {
            disconnect(_dialog);
        }
    }

    virtual QVariant waitForResult() {
        while (!_finished) {
            QCoreApplication::processEvents();
        }
        return _result;
    }

protected slots:
    void onDestroyed() {
        _finished = true;
        disconnect(_dialog);
        _dialog = nullptr;
    }

protected:
    QQuickItem* _dialog;
    bool _finished { false };
    QVariant _result;
};

class MessageBoxListener : public ModalDialogListener {
    Q_OBJECT

    friend class OffscreenUi;
    MessageBoxListener(QQuickItem* messageBox) : ModalDialogListener(messageBox) {
        if (_finished) {
            return;
        }
        connect(_dialog, SIGNAL(selected(int)), this, SLOT(onSelected(int)));
    }

    virtual QMessageBox::StandardButton waitForButtonResult() {
        ModalDialogListener::waitForResult();
        return static_cast<QMessageBox::StandardButton>(_result.toInt());
    }

private slots:
    void onSelected(int button) {
        _result = button;
        _finished = true;
        disconnect(_dialog);
    }
};

QQuickItem* OffscreenUi::createMessageBox(Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    QVariantMap map;
    map.insert("title", title);
    map.insert("text", text);
    map.insert("icon", icon);
    map.insert("buttons", buttons.operator int());
    map.insert("defaultButton", defaultButton);
    QVariant result;
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "messageBox",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, QVariant::fromValue(map)));

    if (!invokeResult) {
        qWarning() << "Failed to create message box";
        return nullptr;
    }
    return qvariant_cast<QQuickItem*>(result);
}

int OffscreenUi::waitForMessageBoxResult(QQuickItem* messageBox) {
    if (!messageBox) {
        return QMessageBox::NoButton;
    }
    
    return MessageBoxListener(messageBox).waitForButtonResult();
}


QMessageBox::StandardButton OffscreenUi::messageBox(Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    if (QThread::currentThread() != thread()) {
        QMessageBox::StandardButton result = QMessageBox::StandardButton::NoButton;
        QMetaObject::invokeMethod(this, "messageBox", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QMessageBox::StandardButton, result),
            Q_ARG(Icon, icon),
            Q_ARG(QString, title),
            Q_ARG(QString, text),
            Q_ARG(QMessageBox::StandardButtons, buttons),
            Q_ARG(QMessageBox::StandardButton, defaultButton));
        return result;
    }

    return static_cast<QMessageBox::StandardButton>(waitForMessageBoxResult(createMessageBox(icon, title, text, buttons, defaultButton)));
}

QMessageBox::StandardButton OffscreenUi::critical(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(OffscreenUi::Icon::ICON_CRITICAL, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::information(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(OffscreenUi::Icon::ICON_INFORMATION, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::question(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(OffscreenUi::Icon::ICON_QUESTION, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::warning(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(OffscreenUi::Icon::ICON_WARNING, title, text, buttons, defaultButton);
}



class InputDialogListener : public ModalDialogListener {
    Q_OBJECT

    friend class OffscreenUi;
    InputDialogListener(QQuickItem* queryBox) : ModalDialogListener(queryBox) {
        if (_finished) {
            return;
        }
        connect(_dialog, SIGNAL(selected(QVariant)), this, SLOT(onSelected(const QVariant&)));
    }

private slots:
    void onSelected(const QVariant& result) {
        _result = result;
        _finished = true;
        disconnect(_dialog);
    }
};

QString OffscreenUi::getText(const Icon icon, const QString& title, const QString& label, const QString& text, bool* ok) {
    if (ok) { *ok = false; }
    QVariant result = DependencyManager::get<OffscreenUi>()->inputDialog(icon, title, label, text).toString();
    if (ok && result.isValid()) {
        *ok = true;
    }
    return result.toString();
}

QString OffscreenUi::getItem(const Icon icon, const QString& title, const QString& label, const QStringList& items,
    int current, bool editable, bool* ok) {

    if (ok) { 
        *ok = false; 
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto inputDialog = offscreenUi->createInputDialog(icon, title, label, current);
    if (!inputDialog) {
        return QString();
    }
    inputDialog->setProperty("items", items);
    inputDialog->setProperty("editable", editable);

    QVariant result = offscreenUi->waitForInputDialogResult(inputDialog);
    if (!result.isValid()) {
        return QString();
    }

    if (ok) {
        *ok = true;
    }
    return result.toString();
}

QVariant OffscreenUi::getCustomInfo(const Icon icon, const QString& title, const QVariantMap& config, bool* ok) {
    if (ok) {
        *ok = false;
    }

    QVariant result = DependencyManager::get<OffscreenUi>()->customInputDialog(icon, title, config);
    if (result.isValid()) {
        // We get a JSON encoded result, so we unpack it into a QVariant wrapping a QVariantMap
        result = QVariant(QJsonDocument::fromJson(result.toString().toUtf8()).object().toVariantMap());
        if (ok) {
            *ok = true;
        }
    }

    return result;
}

QVariant OffscreenUi::inputDialog(const Icon icon, const QString& title, const QString& label, const QVariant& current) {
    if (QThread::currentThread() != thread()) {
        QVariant result;
        QMetaObject::invokeMethod(this, "inputDialog", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVariant, result),
            Q_ARG(Icon, icon),
            Q_ARG(QString, title),
            Q_ARG(QString, label),
            Q_ARG(QVariant, current));
        return result;
    }

    return waitForInputDialogResult(createInputDialog(icon, title, label, current));
}

QVariant OffscreenUi::customInputDialog(const Icon icon, const QString& title, const QVariantMap& config) {
    if (QThread::currentThread() != thread()) {
        QVariant result;
        QMetaObject::invokeMethod(this, "customInputDialog", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(Icon, icon),
                                  Q_ARG(QString, title),
                                  Q_ARG(QVariantMap, config));
        return result;
    }

    return waitForInputDialogResult(createCustomInputDialog(icon, title, config));
}

void OffscreenUi::togglePinned() {
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "togglePinned");
    if (!invokeResult) {
        qWarning() << "Failed to toggle window visibility";
    }
}

void OffscreenUi::setPinned(bool pinned) {
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "setPinned", Q_ARG(QVariant, pinned));
    if (!invokeResult) {
        qWarning() << "Failed to set window visibility";
    }
}

void OffscreenUi::addMenuInitializer(std::function<void(VrMenu*)> f) {
    if (!_vrMenu) {
        _queuedMenuInitializers.push_back(f);
        return;
    }
    f(_vrMenu);
}

QQuickItem* OffscreenUi::createInputDialog(const Icon icon, const QString& title, const QString& label,
    const QVariant& current) {

    QVariantMap map;
    map.insert("title", title);
    map.insert("icon", icon);
    map.insert("label", label);
    map.insert("current", current);
    QVariant result;
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "inputDialog",
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, QVariant::fromValue(map)));

    if (!invokeResult) {
        qWarning() << "Failed to create message box";
        return nullptr;
    }

    return qvariant_cast<QQuickItem*>(result);
}

QQuickItem* OffscreenUi::createCustomInputDialog(const Icon icon, const QString& title, const QVariantMap& config) {
    QVariantMap map = config;
    map.insert("title", title);
    map.insert("icon", icon);
    QVariant result;
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "customInputDialog",
                                                  Q_RETURN_ARG(QVariant, result),
                                                  Q_ARG(QVariant, QVariant::fromValue(map)));

    if (!invokeResult) {
        qWarning() << "Failed to create custom message box";
        return nullptr;
    }

    return qvariant_cast<QQuickItem*>(result);
}

QVariant OffscreenUi::waitForInputDialogResult(QQuickItem* inputDialog) {
    if (!inputDialog) {
        return QVariant();
    }
    return InputDialogListener(inputDialog).waitForResult();
}

bool OffscreenUi::navigationFocused() {
    return !offscreenFlags->isNavigationFocusDisabled() && offscreenFlags->isNavigationFocused();
}

void OffscreenUi::setNavigationFocused(bool focused) {
    offscreenFlags->setNavigationFocused(focused);
}

// FIXME HACK....
// This hack is an attempt to work around the 'offscreen UI can't gain keyboard focus' bug
// https://app.asana.com/0/27650181942747/83176475832393
// The problem seems related to https://bugreports.qt.io/browse/QTBUG-50309 
//
// The workaround seems to be to give some other window (same process or another process doesn't seem to matter)
// focus and then put focus back on the interface main window.  
//
// If I could reliably reproduce this bug I could eventually track down what state change is occuring 
// during the process of the main window losing and then gaining focus, but failing that, here's a 
// brute force way of triggering that state change at application start in a way that should be nearly
// imperceptible to the user.
class KeyboardFocusHack : public QObject {
    Q_OBJECT
public:
    KeyboardFocusHack() {
        Q_ASSERT(_mainWindow);
        QTimer::singleShot(200, [=] {
            _hackWindow = new QWindow();
            _hackWindow->setFlags(Qt::FramelessWindowHint);
            _hackWindow->setGeometry(_mainWindow->x(), _mainWindow->y(), 10, 10);
            _hackWindow->show();
            _hackWindow->requestActivate();
            QTimer::singleShot(200, [=] {
                _hackWindow->hide();
                _hackWindow->deleteLater();
                _hackWindow = nullptr;
                _mainWindow->requestActivate();
                this->deleteLater();
            });
        });
    }

private:
    
    static QWindow* findMainWindow() {
        auto windows = qApp->topLevelWindows();
        QWindow* result = nullptr;
        for (auto window : windows) {
            QVariant isMainWindow = window->property("MainWindow");
            if (!qobject_cast<QQuickWindow*>(window)) {
                result = window;
                break;
            }
        }
        return result;
    }

    QWindow* const _mainWindow { findMainWindow() };
    QWindow* _hackWindow { nullptr };
};

void OffscreenUi::createDesktop(const QUrl& url) {
    if (_desktop) {
        qDebug() << "Desktop already created";
        return;
    }

#ifdef DEBUG
    getRootContext()->setContextProperty("DebugQML", QVariant(true));
#else 
    getRootContext()->setContextProperty("DebugQML", QVariant(false));
#endif

    _desktop = dynamic_cast<QQuickItem*>(load(url));
    Q_ASSERT(_desktop);
    getRootContext()->setContextProperty("desktop", _desktop);

    _toolWindow = _desktop->findChild<QQuickItem*>("ToolWindow");

    _vrMenu = new VrMenu(this);
    for (const auto& menuInitializer : _queuedMenuInitializers) {
        menuInitializer(_vrMenu);
    }

    new KeyboardFocusHack();

    connect(_desktop, SIGNAL(showDesktop()), this, SIGNAL(showDesktop()));
}

QQuickItem* OffscreenUi::getDesktop() {
    return _desktop;
}

QQuickItem* OffscreenUi::getToolWindow() {
    return _toolWindow;
}

void OffscreenUi::unfocusWindows() {
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "unfocusWindows");
    Q_ASSERT(invokeResult);
}

void OffscreenUi::toggleMenu(const QPoint& screenPosition) { // caller should already have mapped using getReticlePosition
    emit showDesktop(); // we really only want to do this if you're showing the menu, but for now this works
    QMetaObject::invokeMethod(_desktop, "toggleMenu", Q_ARG(QVariant, screenPosition));
}


class FileDialogListener : public ModalDialogListener {
    Q_OBJECT

    friend class OffscreenUi;
    FileDialogListener(QQuickItem* messageBox) : ModalDialogListener(messageBox) {
        if (_finished) {
            return;
        }
        connect(_dialog, SIGNAL(selectedFile(QVariant)), this, SLOT(onSelectedFile(QVariant)));
    }

private slots:
    void onSelectedFile(QVariant file) {
        _result = file;
        _finished = true;
        disconnect(_dialog);
    }
};


QString OffscreenUi::fileDialog(const QVariantMap& properties) {
    QVariant buildDialogResult;
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "fileDialog",
        Q_RETURN_ARG(QVariant, buildDialogResult),
        Q_ARG(QVariant, QVariant::fromValue(properties)));

    if (!invokeResult) {
        qWarning() << "Failed to create file open dialog";
        return QString();
    }

    QVariant result = FileDialogListener(qvariant_cast<QQuickItem*>(buildDialogResult)).waitForResult();
    if (!result.isValid()) {
        return QString();
    }
    qDebug() << result.toString();
    return result.toUrl().toLocalFile();
}

QString OffscreenUi::fileOpenDialog(const QString& caption, const QString& dir, const QString& filter, QString* selectedFilter, QFileDialog::Options options) {
    if (QThread::currentThread() != thread()) {
        QString result;
        QMetaObject::invokeMethod(this, "fileOpenDialog", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QString, result),
            Q_ARG(QString, caption),
            Q_ARG(QString, dir),
            Q_ARG(QString, filter),
            Q_ARG(QString*, selectedFilter),
            Q_ARG(QFileDialog::Options, options));
        return result;
    }

    // FIXME support returning the selected filter... somehow?
    QVariantMap map;
    map.insert("caption", caption);
    map.insert("dir", QUrl::fromLocalFile(dir));
    map.insert("filter", filter);
    map.insert("options", static_cast<int>(options));
    return fileDialog(map);
}

QString OffscreenUi::fileSaveDialog(const QString& caption, const QString& dir, const QString& filter, QString* selectedFilter, QFileDialog::Options options) {
    if (QThread::currentThread() != thread()) {
        QString result;
        QMetaObject::invokeMethod(this, "fileSaveDialog", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QString, result),
            Q_ARG(QString, caption),
            Q_ARG(QString, dir),
            Q_ARG(QString, filter),
            Q_ARG(QString*, selectedFilter),
            Q_ARG(QFileDialog::Options, options));
        return result;
    }

    // FIXME support returning the selected filter... somehow?
    QVariantMap map;
    map.insert("caption", caption);
    map.insert("dir", QUrl::fromLocalFile(dir));
    map.insert("filter", filter);
    map.insert("options", static_cast<int>(options));
    map.insert("saveDialog", true);

    return fileDialog(map);
}

QString OffscreenUi::getOpenFileName(void* ignored, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options) {
    return DependencyManager::get<OffscreenUi>()->fileOpenDialog(caption, dir, filter, selectedFilter, options);
}

QString OffscreenUi::getSaveFileName(void* ignored, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options) {
    return DependencyManager::get<OffscreenUi>()->fileSaveDialog(caption, dir, filter, selectedFilter, options);
}

bool OffscreenUi::eventFilter(QObject* originalDestination, QEvent* event) {
    if (!filterEnabled(originalDestination, event)) {
        return false;
    }

    // let the parent class do it's work
    bool result = OffscreenQmlSurface::eventFilter(originalDestination, event);


    // Check if this is a key press/release event that might need special attention
    auto type = event->type();
    if (type != QEvent::KeyPress && type != QEvent::KeyRelease) {
        return result;
    }

    QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
    auto key = keyEvent->key();
    bool& pressed = _pressedKeys[key];

    // Keep track of which key press events the QML has accepted
    if (result && QEvent::KeyPress == type) {
        pressed = true;
    }

    // QML input elements absorb key press, but apparently not key release.
    // therefore we want to ensure that key release events for key presses that were 
    // accepted by the QML layer are suppressed
    if (type == QEvent::KeyRelease && pressed) {
        pressed = false;
        return true;
    }

    return result;
}

unsigned int OffscreenUi::getMenuUserDataId() const {
    return _vrMenu->_userDataId;
}

#include "OffscreenUi.moc"

