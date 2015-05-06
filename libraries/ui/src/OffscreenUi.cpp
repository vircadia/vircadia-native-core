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
#include <QOpenGLFramebufferObject>
#include <QOpenGLDebugLogger>
#include <QGLWidget>
#include <QtQml>
#include "MessageDialog.h"


Q_DECLARE_LOGGING_CATEGORY(offscreenFocus)
Q_LOGGING_CATEGORY(offscreenFocus, "hifi.offscreen.focus")

// Time between receiving a request to render the offscreen UI actually triggering
// the render.  Could possibly be increased depending on the framerate we expect to
// achieve.
static const int SMALL_INTERVAL = 5;

class OffscreenUiRoot : public QQuickItem {
    Q_OBJECT
public:

    OffscreenUiRoot(QQuickItem* parent = 0);
    Q_INVOKABLE void information(const QString& title, const QString& text);
    Q_INVOKABLE void loadChild(const QUrl& url) {
        DependencyManager::get<OffscreenUi>()->load(url);
    }
};


OffscreenUiRoot::OffscreenUiRoot(QQuickItem* parent) : QQuickItem(parent) {
}

void OffscreenUiRoot::information(const QString& title, const QString& text) {
    OffscreenUi::information(title, text);
}

OffscreenUi::OffscreenUi() {
    ::qmlRegisterType<OffscreenUiRoot>("Hifi", 1, 0, "Root");
}

OffscreenUi::~OffscreenUi() {
    // Make sure the context is current while doing cleanup. Note that we use the
    // offscreen surface here because passing 'this' at this point is not safe: the
    // underlying platform window may already be destroyed. To avoid all the trouble, use
    // another surface that is valid for sure.
    makeCurrent();

    // Delete the render control first since it will free the scenegraph resources.
    // Destroy the QQuickWindow only afterwards.
    delete _renderControl;

    delete _qmlComponent;
    delete _quickWindow;
    delete _qmlEngine;

    doneCurrent();
}

void OffscreenUi::create(QOpenGLContext* shareContext) {
    OffscreenGlCanvas::create(shareContext);

    makeCurrent();

    // Create a QQuickWindow that is associated with out render control. Note that this
    // window never gets created or shown, meaning that it will never get an underlying
    // native (platform) window.
    QQuickWindow::setDefaultAlphaBuffer(true);
    _quickWindow = new QQuickWindow(_renderControl);
    _quickWindow->setColor(QColor(255, 255, 255, 0));
    _quickWindow->setFlags(_quickWindow->flags() | static_cast<Qt::WindowFlags>(Qt::WA_TranslucentBackground));
    // Create a QML engine.
    _qmlEngine = new QQmlEngine;
    if (!_qmlEngine->incubationController()) {
        _qmlEngine->setIncubationController(_quickWindow->incubationController());
    }

    // When Quick says there is a need to render, we will not render immediately. Instead,
    // a timer with a small interval is used to get better performance.
    _updateTimer.setSingleShot(true);
    _updateTimer.setInterval(SMALL_INTERVAL);
    connect(&_updateTimer, &QTimer::timeout, this, &OffscreenUi::updateQuick);

    // Now hook up the signals. For simplicy we don't differentiate between
    // renderRequested (only render is needed, no sync) and sceneChanged (polish and sync
    // is needed too).
    connect(_renderControl, &QQuickRenderControl::renderRequested, this, &OffscreenUi::requestRender);
    connect(_renderControl, &QQuickRenderControl::sceneChanged, this, &OffscreenUi::requestUpdate);

#ifdef DEBUG
    connect(_quickWindow, &QQuickWindow::focusObjectChanged, [this]{
        qCDebug(offscreenFocus) << "New focus item " << _quickWindow->focusObject();
    });
    connect(_quickWindow, &QQuickWindow::activeFocusItemChanged, [this] {
        qCDebug(offscreenFocus) << "New active focus item " << _quickWindow->activeFocusItem();
    });
#endif

    _qmlComponent = new QQmlComponent(_qmlEngine);
    // Initialize the render control and our OpenGL resources.
    makeCurrent();
    _renderControl->initialize(&_context);
}

void OffscreenUi::addImportPath(const QString& path) {
    _qmlEngine->addImportPath(path);
}

void OffscreenUi::resize(const QSize& newSize) {
    makeCurrent();

    // Clear out any fbos with the old size
    qreal pixelRatio = _renderControl->_renderWindow ? _renderControl->_renderWindow->devicePixelRatio() : 1.0;
    qDebug() << "Offscreen UI resizing to " << newSize.width() << "x" << newSize.height() << " with pixel ratio " << pixelRatio;
    _fboCache.setSize(newSize * pixelRatio);

    if (_quickWindow) {
        _quickWindow->setGeometry(QRect(QPoint(), newSize));
        _quickWindow->contentItem()->setSize(newSize);
    }


    // Update our members
    if (_rootItem) {
        _rootItem->setSize(newSize);
    }

    doneCurrent();
}

QQuickItem* OffscreenUi::getRootItem() {
    return _rootItem;
}

void OffscreenUi::setBaseUrl(const QUrl& baseUrl) {
    _qmlEngine->setBaseUrl(baseUrl);
}

QObject* OffscreenUi::load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f) {
    _qmlComponent->loadUrl(qmlSource);
    if (_qmlComponent->isLoading()) {
        connect(_qmlComponent, &QQmlComponent::statusChanged, this, 
            [this, f](QQmlComponent::Status){
                finishQmlLoad(f);
            });
        return nullptr;
    }
    
    return finishQmlLoad(f);
}

void OffscreenUi::requestUpdate() {
    _polish = true;
    if (!_updateTimer.isActive()) {
        _updateTimer.start();
    }
}

void OffscreenUi::requestRender() {
    if (!_updateTimer.isActive()) {
        _updateTimer.start();
    }
}

QObject* OffscreenUi::finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f) {
    disconnect(_qmlComponent, &QQmlComponent::statusChanged, this, 0);
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError& error, errorList) {
            qWarning() << error.url() << error.line() << error;
        }
        return nullptr;
    }

    QQmlContext* newContext = new QQmlContext(_qmlEngine, qApp);
    QObject* newObject = _qmlComponent->beginCreate(newContext);
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError& error, errorList)
            qWarning() << error.url() << error.line() << error;
        if (!_rootItem) {
            qFatal("Unable to finish loading QML root");
        }
        return nullptr;
    }

    f(newContext, newObject);
    _qmlComponent->completeCreate();


    // All quick items should be focusable
    QQuickItem* newItem = qobject_cast<QQuickItem*>(newObject);
    if (newItem) {
        // Make sure we make items focusable (critical for 
        // supporting keyboard shortcuts)
        newItem->setFlag(QQuickItem::ItemIsFocusScope, true);
    }

    // If we already have a root, just set a couple of flags and the ancestry
    if (_rootItem) {
        // Allow child windows to be destroyed from JS
        QQmlEngine::setObjectOwnership(newObject, QQmlEngine::JavaScriptOwnership);
        newObject->setParent(_rootItem);
        if (newItem) {
            newItem->setParentItem(_rootItem);
        }
        return newObject;
    }

    if (!newItem) {
        qFatal("Could not load object as root item");
        return nullptr;
    }
    // The root item is ready. Associate it with the window.
    _rootItem = newItem;
    _rootItem->setParentItem(_quickWindow->contentItem());
    _rootItem->setSize(_quickWindow->renderTargetSize());
    return _rootItem;
}


void OffscreenUi::updateQuick() {
    if (_paused) {
        return;
    }
    if (!makeCurrent()) {
        return;
    }

    // Polish, synchronize and render the next frame (into our fbo).  In this example
    // everything happens on the same thread and therefore all three steps are performed
    // in succession from here. In a threaded setup the render() call would happen on a
    // separate thread.
    if (_polish) {
        _renderControl->polishItems();
        _renderControl->sync();
        _polish = false;
    }

    QOpenGLFramebufferObject* fbo = _fboCache.getReadyFbo();

    _quickWindow->setRenderTarget(fbo);
    fbo->bind();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _renderControl->render();
    // FIXME The web browsers seem to be leaving GL in an error state.
    // Need a debug context with sync logging to figure out why.
    // for now just clear the errors
    glGetError();
//    Q_ASSERT(!glGetError());

    _quickWindow->resetOpenGLState();

    QOpenGLFramebufferObject::bindDefault();
    // Force completion of all the operations before we emit the texture as being ready for use
    glFinish();

    emit textureUpdated(fbo->texture());
}

QPointF OffscreenUi::mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject) {
    vec2 sourceSize;
    if (dynamic_cast<QWidget*>(sourceObject)) {
        sourceSize = toGlm(((QWidget*)sourceObject)->size());
    } else if (dynamic_cast<QWindow*>(sourceObject)) {
        sourceSize = toGlm(((QWindow*)sourceObject)->size());
    }
    vec2 offscreenPosition = toGlm(sourcePosition);
    offscreenPosition /= sourceSize;
    offscreenPosition *= vec2(toGlm(_quickWindow->size()));
    return QPointF(offscreenPosition.x, offscreenPosition.y);
}

// This hack allows the QML UI to work with keys that are also bound as 
// shortcuts at the application level.  However, it seems as though the 
// bound actions are still getting triggered.  At least for backspace.  
// Not sure why.
//
// However, the problem may go away once we switch to the new menu system,
// so I think it's OK for the time being.
bool OffscreenUi::shouldSwallowShortcut(QEvent* event) {
    Q_ASSERT(event->type() == QEvent::ShortcutOverride);
    QObject* focusObject = _quickWindow->focusObject();
    if (focusObject != _quickWindow && focusObject != _rootItem) {
        //qDebug() << "Swallowed shortcut " << static_cast<QKeyEvent*>(event)->key();
        event->accept();
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////
//
// Event handling customization
//

bool OffscreenUi::eventFilter(QObject* originalDestination, QEvent* event) {
    // Only intercept events while we're in an active state
    if (_paused) {
        return false;
    }


#ifdef DEBUG
    // Don't intercept our own events, or we enter an infinite recursion
    QObject* recurseTest = originalDestination;
    while (recurseTest) {
        Q_ASSERT(recurseTest != _rootItem && recurseTest != _quickWindow);
        recurseTest = recurseTest->parent();
    }
#endif

   
    switch (event->type()) {
        case QEvent::Resize: {
            QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
            QGLWidget* widget = dynamic_cast<QGLWidget*>(originalDestination);
            if (widget) {
                this->resize(resizeEvent->size());
            }
            break;
        }

        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            event->ignore();
            if (QCoreApplication::sendEvent(_quickWindow, event)) {
                return event->isAccepted();
            }
            break;
        }

        case QEvent::Wheel: {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            QWheelEvent mappedEvent(
                    mapWindowToUi(wheelEvent->pos(), originalDestination),
                    wheelEvent->delta(),  wheelEvent->buttons(),
                    wheelEvent->modifiers(), wheelEvent->orientation());
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_quickWindow, &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }

        // Fall through
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF originalPos = mouseEvent->localPos();
            QPointF transformedPos = _mouseTranslator(originalPos);
            transformedPos = mapWindowToUi(transformedPos, originalDestination);
            QMouseEvent mappedEvent(mouseEvent->type(),
                    transformedPos,
                    mouseEvent->screenPos(), mouseEvent->button(),
                    mouseEvent->buttons(), mouseEvent->modifiers());
            if (event->type() == QEvent::MouseMove) {
                _qmlEngine->rootContext()->setContextProperty("lastMousePosition", transformedPos);
            }
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_quickWindow, &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }

        default:
            break;
    }

    return false;
}

void OffscreenUi::lockTexture(int texture) {
    _fboCache.lockTexture(texture);
}

void OffscreenUi::releaseTexture(int texture) {
    _fboCache.releaseTexture(texture);
}

void OffscreenUi::pause() {
    _paused = true;
}

void OffscreenUi::resume() {
    _paused = false;
    requestRender();
}

bool OffscreenUi::isPaused() const {
    return _paused;
}

void OffscreenUi::setProxyWindow(QWindow* window) {
    _renderControl->_renderWindow = window;
}

void OffscreenUi::show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = _rootItem->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = _rootItem->findChild<QQuickItem*>(name); 
    }
    if (item) {
        item->setEnabled(true);
    }
}

void OffscreenUi::toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = _rootItem->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = _rootItem->findChild<QQuickItem*>(name);
    }
    if (item) {
        item->setEnabled(!item->isEnabled());
    }
}

void OffscreenUi::messageBox(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::Icon icon,
    QMessageBox::StandardButtons buttons) {
    MessageDialog* pDialog{ nullptr };
    MessageDialog::show([&](QQmlContext* ctx, QObject* item) {
        pDialog = item->findChild<MessageDialog*>();
        pDialog->setIcon((MessageDialog::Icon)icon);
        pDialog->setTitle(title);
        pDialog->setText(text);
        pDialog->setStandardButtons(MessageDialog::StandardButtons(static_cast<int>(buttons)));
        pDialog->setResultCallback(callback);
    });
    pDialog->setEnabled(true);
}

void OffscreenUi::information(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Information), buttons);
}

void OffscreenUi::question(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Question), buttons);
}

void OffscreenUi::warning(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Warning), buttons);
}

void OffscreenUi::critical(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Critical), buttons);
}


OffscreenUi::ButtonCallback OffscreenUi::NO_OP_CALLBACK = [](QMessageBox::StandardButton) {};

#include "OffscreenUi.moc"
