#include "OffscreenUi.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLDebugLogger>
#include <QGLWidget>

OffscreenUi::OffscreenUi() {
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

void OffscreenUi::create(QOpenGLContext * shareContext) {
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
    if (!_qmlEngine->incubationController())
        _qmlEngine->setIncubationController(_quickWindow->incubationController());

    // When Quick says there is a need to render, we will not render immediately. Instead,
    // a timer with a small interval is used to get better performance.
    _updateTimer.setSingleShot(true);
    _updateTimer.setInterval(5);
    connect(&_updateTimer, &QTimer::timeout, this, &OffscreenUi::updateQuick);

    // Now hook up the signals. For simplicy we don't differentiate between
    // renderRequested (only render is needed, no sync) and sceneChanged (polish and sync
    // is needed too).
    connect(_renderControl, &QQuickRenderControl::renderRequested, this, &OffscreenUi::requestRender);
    connect(_renderControl, &QQuickRenderControl::sceneChanged, this, &OffscreenUi::requestUpdate);
    connect(_quickWindow, &QQuickWindow::focusObjectChanged, this, [this](QObject *object){
        OffscreenUi * p = this;
        qDebug() << "Focus changed to " << object;
    });
    _quickWindow->focusObject();

    _qmlComponent = new QQmlComponent(_qmlEngine);

    // Initialize the render control and our OpenGL resources.
    makeCurrent();
    _renderControl->initialize(&_context);
}

void OffscreenUi::resize(const QSize & newSize) {
    makeCurrent();

    // Clear out any fbos with the old size
    _fboCache.setSize(newSize);

    // Update our members
    if (_rootItem) {
        _rootItem->setSize(newSize);
    }

    if (_quickWindow) {
        _quickWindow->setGeometry(QRect(QPoint(), newSize));
    }

    doneCurrent();
}

QQmlContext * OffscreenUi::qmlContext() {
    if (nullptr == _rootItem) {
        return _qmlComponent->creationContext();
    }
    return QQmlEngine::contextForObject(_rootItem);
}

void OffscreenUi::loadQml(const QUrl & qmlSource, std::function<void(QQmlContext*)> f) {
    _qmlComponent->loadUrl(qmlSource);
    if (_qmlComponent->isLoading())
        connect(_qmlComponent, &QQmlComponent::statusChanged, this, &OffscreenUi::finishQmlLoad);
    else
        finishQmlLoad();
}

void OffscreenUi::requestUpdate() {
    _polish = true;
    if (!_updateTimer.isActive())
        _updateTimer.start();
}

void OffscreenUi::requestRender() {
    if (!_updateTimer.isActive())
        _updateTimer.start();
}

void OffscreenUi::finishQmlLoad() {
    disconnect(_qmlComponent, &QQmlComponent::statusChanged, this, &OffscreenUi::finishQmlLoad);
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError &error, errorList) {
            qWarning() << error.url() << error.line() << error;
        }
        return;
    }

    QObject *rootObject = _qmlComponent->create();
    if (_qmlComponent->isError()) {
        QList<QQmlError> errorList = _qmlComponent->errors();
        foreach(const QQmlError &error, errorList)
            qWarning() << error.url() << error.line() << error;
        qFatal("Unable to finish loading QML");
        return;
    }

    _rootItem = qobject_cast<QQuickItem *>(rootObject);
    if (!_rootItem) {
        qWarning("run: Not a QQuickItem");
        delete rootObject;
        qFatal("Unable to find root QQuickItem");
        return;
    }

    // Make sure we can assign focus to the root item (critical for 
    // supporting keyboard shortcuts)
    _rootItem->setFlag(QQuickItem::ItemIsFocusScope, true);
    // The root item is ready. Associate it with the window.
    _rootItem->setParentItem(_quickWindow->contentItem());
    _rootItem->setSize(_quickWindow->renderTargetSize());
    qDebug() << "Finished setting up QML provider";
}


void OffscreenUi::updateQuick() {
    if (_paused) {
        return;
    }
    if (!makeCurrent())
        return;

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

    Q_ASSERT(!glGetError());

    _quickWindow->resetOpenGLState();

    QOpenGLFramebufferObject::bindDefault();
    // Force completion of all the operations before we emit the texture as being ready for use
    glFinish();

    emit textureUpdated(fbo->texture());
}

QPointF OffscreenUi::mapWindowToUi(const QPointF & p, QObject * dest) {
    vec2 sourceSize;
    if (dynamic_cast<QWidget*>(dest)) {
        sourceSize = toGlm(((QWidget*)dest)->size());
    } else if (dynamic_cast<QWindow*>(dest)) {
        sourceSize = toGlm(((QWindow*)dest)->size());
    }
    vec2 pos = toGlm(p);
    pos /= sourceSize;
    pos *= vec2(toGlm(_quickWindow->renderTargetSize()));
    return QPointF(pos.x, pos.y);
}

///////////////////////////////////////////////////////
//
// Event handling customization
//

bool OffscreenUi::eventFilter(QObject * dest, QEvent * e) {
    // Only intercept events while we're in an active state
    if (_paused) {
        return false;
    }

    // Don't intercept our own events, or we enter an infinite recursion
    if (dest == _quickWindow) {
        return false;
    }
    
    switch (e->type()) {
    case QEvent::Resize:
    {
        QResizeEvent * re = (QResizeEvent *)e;
        QGLWidget * widget = dynamic_cast<QGLWidget*>(dest);
        if (widget) {
            this->resize(re->size());
        }
        return false;
    }

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        e->ignore();
        if (QApplication::sendEvent(_quickWindow, e)) {
            return e->isAccepted();
        }
    }
    break;

    case QEvent::Wheel:
    {
        QWheelEvent * we = (QWheelEvent*)e;
        QWheelEvent mappedEvent(mapWindowToUi(we->pos(), dest), we->delta(), we->buttons(), we->modifiers(), we->orientation());
        QCoreApplication::sendEvent(_quickWindow, &mappedEvent);
        return true;
    }
    break;

    // Fall through
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    {
        QMouseEvent * me = (QMouseEvent *)e;
        QPointF originalPos = me->localPos();
        QPointF transformedPos = _mouseTranslator(originalPos);
        QMouseEvent mappedEvent(e->type(), mapWindowToUi(transformedPos, dest), me->screenPos(), me->button(), me->buttons(), me->modifiers());
        QCoreApplication::sendEvent(_quickWindow, &mappedEvent);
        return QObject::event(e);
    }

    default: break;
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

void OffscreenUi::setProxyWindow(QWindow * window) {
    _renderControl->_renderWindow = window;
}

void OffscreenUi::show(const QUrl & url, const QString & name) {
    QQuickItem * item = _rootItem->findChild<QQuickItem*>(name);
    if (nullptr != item) {
        item->setEnabled(true);
        item->setVisible(true);
    } else {
        load(url);
    }
}

void OffscreenUi::toggle(const QUrl & url, const QString & name) {
    QQuickItem * item = _rootItem->findChild<QQuickItem*>(name);
    // First load?
    if (nullptr == item) {
        load(url);
        return;
    }

    // Toggle the visibity AND the enabled flag (otherwise invisible 
    // dialogs can still swallow keyboard input)
    bool newFlag = !item->isVisible();
    item->setVisible(newFlag);
    item->setEnabled(newFlag);
}


void OffscreenUi::load(const QUrl & url) {
    QVariant returnedValue;
    QVariant msg = url;
    QMetaObject::invokeMethod(_rootItem, "loadChild",
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, msg));
    qDebug() << "QML function returned:" << returnedValue.toString();
}

