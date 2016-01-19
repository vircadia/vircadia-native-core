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

#include <QtQml/QtQml>
#include <QtQuick/QQuickWindow>

#include <AbstractUriHandler.h>
#include <AccountManager.h>

// Needs to match the constants in resources/qml/Global.js
class OffscreenFlags : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool navigationFocused READ isNavigationFocused WRITE setNavigationFocused NOTIFY navigationFocusedChanged)

public:

    OffscreenFlags(QObject* parent = nullptr) : QObject(parent) {}
    bool isNavigationFocused() const { return _navigationFocused; }
    void setNavigationFocused(bool focused) {
        if (_navigationFocused != focused) {
            _navigationFocused = focused;
            emit navigationFocusedChanged();
        }
    }

signals:
    void navigationFocusedChanged();

private:
    bool _navigationFocused { false };
};

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
        static const QString ACCESS_TOKEN_PARAMETER = "access_token";
        static const QString ALLOWED_HOST = "metaverse.highfidelity.com";
        QString result = originalUrl;
        QUrl url(originalUrl);
        QUrlQuery query(url);
        if (url.host() == ALLOWED_HOST && query.allQueryItemValues(ACCESS_TOKEN_PARAMETER).empty()) {
            qDebug() << "Updating URL with auth token";
            AccountManager& accountManager = AccountManager::getInstance();
            query.addQueryItem(ACCESS_TOKEN_PARAMETER, accountManager.getAccountInfo().getAccessToken().token);
            url.setQuery(query.query());
            result = url.toString();
        }

        return result;
    }
};

static UrlHandler * urlHandler { nullptr };
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

void OffscreenUi::create(QOpenGLContext* context) {
    OffscreenQmlSurface::create(context);
    auto rootContext = getRootContext();

    offscreenFlags = new OffscreenFlags();
    rootContext->setContextProperty("offscreenFlags", offscreenFlags);
    urlHandler = new UrlHandler();
    rootContext->setContextProperty("urlHandler", urlHandler);
}

void OffscreenUi::show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = getRootItem()->findChild<QQuickItem*>(name);
    }
    if (item) {
        item->setVisible(true);
    }
}

void OffscreenUi::toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = getRootItem()->findChild<QQuickItem*>(name);
    }
    if (item) {
        item->setVisible(!item->isVisible());
    }
}

class MessageBoxListener : public QObject {
    Q_OBJECT

    friend class OffscreenUi;

    MessageBoxListener(QQuickItem* messageBox) : _messageBox(messageBox) {
        if (!_messageBox) {
            _finished = true;
            return;
        } 
        connect(_messageBox, SIGNAL(selected(int)), this, SLOT(onSelected(int)));
        connect(_messageBox, SIGNAL(destroyed()), this, SLOT(onDestroyed()));
    }

    ~MessageBoxListener() {
        disconnect(_messageBox);
    }

    QMessageBox::StandardButton waitForResult() {
        while (!_finished) {
            QCoreApplication::processEvents();
        }
        return _result;
    }

private slots:
    void onSelected(int button) {
        _result = static_cast<QMessageBox::StandardButton>(button);
        _finished = true;
        disconnect(_messageBox);
    }

    void onDestroyed() {
        _finished = true;
        disconnect(_messageBox);
    }

private:
    bool _finished { false };
    QMessageBox::StandardButton _result { QMessageBox::StandardButton::NoButton };
    QQuickItem* const _messageBox;
};

QMessageBox::StandardButton OffscreenUi::messageBox(QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    if (QThread::currentThread() != thread()) {
        QMessageBox::StandardButton result = QMessageBox::StandardButton::NoButton;
        QMetaObject::invokeMethod(this, "messageBox", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QMessageBox::StandardButton, result),
            Q_ARG(QMessageBox::Icon, icon),
            Q_ARG(QString, title),
            Q_ARG(QString, text),
            Q_ARG(QMessageBox::StandardButtons, buttons),
            Q_ARG(QMessageBox::StandardButton, defaultButton));
        return result;
    }

    QVariantMap map;
    map.insert("title", title);
    map.insert("text", text);
    map.insert("icon", icon);
    map.insert("buttons", buttons.operator int());
    map.insert("defaultButton", defaultButton);
    QVariant result;
    bool invokeResult = QMetaObject::invokeMethod(getDesktop(), "messageBox", 
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, QVariant::fromValue(map)));

    if (!invokeResult) {
        qWarning() << "Failed to create message box";
        return QMessageBox::StandardButton::NoButton;
    }
    
    auto resultButton = MessageBoxListener(qvariant_cast<QQuickItem*>(result)).waitForResult();
    qDebug() << "Message box got a result of " << resultButton;
    return resultButton;
}

QMessageBox::StandardButton OffscreenUi::critical(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::information(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::question(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}
QMessageBox::StandardButton OffscreenUi::warning(const QString& title, const QString& text,
    QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
    return DependencyManager::get<OffscreenUi>()->messageBox(QMessageBox::Icon::Critical, title, text, buttons, defaultButton);
}

bool OffscreenUi::navigationFocused() {
    return offscreenFlags->isNavigationFocused();
}

void OffscreenUi::setNavigationFocused(bool focused) {
    offscreenFlags->setNavigationFocused(focused);
}

void OffscreenUi::createDesktop() {
    if (_desktop) {
        qDebug() << "Desktop already created";
    }
    getRootContext()->setContextProperty("DebugQML", QVariant(false));
    _desktop = dynamic_cast<QQuickItem*>(load("Root.qml"));
    Q_ASSERT(_desktop);
    getRootContext()->setContextProperty("Desktop", _desktop);
    _desktop->setProperty("offscreenWindow", QVariant::fromValue(getWindow()));
    _toolWindow = _desktop->findChild<QQuickItem*>("ToolWindow");
}

QQuickItem* OffscreenUi::getDesktop() {
    return _desktop;
}

QQuickItem* OffscreenUi::getToolWindow() {
    return _toolWindow;
}

Q_DECLARE_METATYPE(std::function<void()>);
static auto VoidLambdaType = qRegisterMetaType<std::function<void()>>();
Q_DECLARE_METATYPE(std::function<QVariant()>);
static auto VariantLambdaType = qRegisterMetaType<std::function<QVariant()>>();


void OffscreenUi::executeOnUiThread(std::function<void()> function) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "executeOnUiThread", Qt::QueuedConnection,
            Q_ARG(std::function<void()>, function));
        return;
    }

    function();
}

QVariant OffscreenUi::returnFromUiThread(std::function<QVariant()> function) {
    if (QThread::currentThread() != thread()) {
        QVariant result;
        QMetaObject::invokeMethod(this, "returnFromUiThread", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVariant, result),
            Q_ARG(std::function<QVariant()>, function));
        return result;
    }

    return function();
}

void OffscreenUi::unfocusWindows() {
    bool invokeResult = QMetaObject::invokeMethod(_desktop, "unfocusWindows");
    Q_ASSERT(invokeResult);
}


#include "OffscreenUi.moc"
