//
//  TextOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextOverlay.h"

#include <QQuickItem>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <GLMHelpers.h>
#include <gpu/GLBackend.h>
#include <OffscreenUi.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>
#include <TextureCache.h>
#include <ViewFrustum.h>

#include "Application.h"
#include "text/FontFamilies.h"

#define TEXT_OVERLAY_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name WRITE set##name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
    void set##name(const type& name) { \
        if (name != _##name) { \
            _##name = name; \
            emit name##Changed(); \
        } \
    } \
private: \
    type _##name{ initialValue }; 


class TextOverlayElement : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL
private:
    TEXT_OVERLAY_PROPERTY(QString, text, "")
    TEXT_OVERLAY_PROPERTY(QString, fontFamily, SANS_FONT_FAMILY)
    TEXT_OVERLAY_PROPERTY(QString, textColor, "#ffffffff")
    TEXT_OVERLAY_PROPERTY(QString, backgroundColor, "#B2000000")
    TEXT_OVERLAY_PROPERTY(qreal, fontSize, 18)
    TEXT_OVERLAY_PROPERTY(qreal, lineHeight, 18)
    TEXT_OVERLAY_PROPERTY(qreal, leftMargin, 0)
    TEXT_OVERLAY_PROPERTY(qreal, topMargin, 0)

public:
    TextOverlayElement(QQuickItem* parent = nullptr) : QQuickItem(parent) {
    }

signals:
    void textChanged();
    void fontFamilyChanged();
    void fontSizeChanged();
    void lineHeightChanged();
    void leftMarginChanged();
    void topMarginChanged();
    void textColorChanged();
    void backgroundColorChanged();
};

HIFI_QML_DEF(TextOverlayElement)

QString toQmlColor(const glm::vec4& v) {
    QString templat("#%1%2%3%4");
    return templat.
        arg((int)(v.a * 255), 2, 16, QChar('0')).
        arg((int)(v.r * 255), 2, 16, QChar('0')).
        arg((int)(v.g * 255), 2, 16, QChar('0')).
        arg((int)(v.b * 255), 2, 16, QChar('0'));
}

TextOverlay::TextOverlay() :
    _backgroundColor(DEFAULT_BACKGROUND_COLOR),
    _backgroundAlpha(DEFAULT_BACKGROUND_ALPHA),
    _leftMargin(DEFAULT_MARGIN),
    _topMargin(DEFAULT_MARGIN),
    _fontSize(DEFAULT_FONTSIZE)
{

    qApp->postLambdaEvent([=] {
        static std::once_flag once;
        std::call_once(once, [] {
            TextOverlayElement::registerType();
        });
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        TextOverlayElement::show([=](QQmlContext* context, QObject* object) {
            _qmlElement = static_cast<TextOverlayElement*>(object);
        });
    });
    while (!_qmlElement) {
        QThread::msleep(1);
    }
}

TextOverlay::TextOverlay(const TextOverlay* textOverlay) :
    Overlay2D(textOverlay),
    _text(textOverlay->_text),
    _backgroundColor(textOverlay->_backgroundColor),
    _backgroundAlpha(textOverlay->_backgroundAlpha),
    _leftMargin(textOverlay->_leftMargin),
    _topMargin(textOverlay->_topMargin),
    _fontSize(textOverlay->_fontSize)
{
    qApp->postLambdaEvent([=] {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        TextOverlayElement::show([this](QQmlContext* context, QObject* object) {
            _qmlElement = static_cast<TextOverlayElement*>(object);
        });
    });
    while (!_qmlElement) {
        QThread::sleep(1);
    }
}

TextOverlay::~TextOverlay() {
    if (_qmlElement) {
        _qmlElement->deleteLater();
    }
}

xColor TextOverlay::getBackgroundColor() {
    if (_colorPulse == 0.0f) {
        return _backgroundColor; 
    }

    float pulseLevel = updatePulse();
    xColor result = _backgroundColor;
    if (_colorPulse < 0.0f) {
        result.red *= (1.0f - pulseLevel);
        result.green *= (1.0f - pulseLevel);
        result.blue *= (1.0f - pulseLevel);
    } else {
        result.red *= pulseLevel;
        result.green *= pulseLevel;
        result.blue *= pulseLevel;
    }
    return result;
}

void TextOverlay::render(RenderArgs* args) {
    if (_visible != _qmlElement->isVisible()) {
        _qmlElement->setVisible(_visible);
    }
    float pulseLevel = updatePulse();
    static float _oldPulseLevel = 0.0f;
    if (pulseLevel != _oldPulseLevel) {

    }
}


void TextOverlay::setProperties(const QScriptValue& properties) {
    Overlay2D::setProperties(properties);
    _qmlElement->setX(_bounds.left());
    _qmlElement->setY(_bounds.top());
    _qmlElement->setWidth(_bounds.width());
    _qmlElement->setHeight(_bounds.height());
    _qmlElement->settextColor(toQmlColor(vec4(toGlm(_color), _alpha)));
    QScriptValue font = properties.property("font");
    if (font.isObject()) {
        if (font.property("size").isValid()) {
            setFontSize(font.property("size").toInt32());
        }
        QFont font(_qmlElement->fontFamily());
        font.setPixelSize(_qmlElement->fontSize());
        QFontMetrics fm(font);
        _qmlElement->setlineHeight(fm.lineSpacing() * 1.2);
    }

    QScriptValue text = properties.property("text");
    if (text.isValid()) {
        setText(text.toVariant().toString());
    }

    QScriptValue backgroundColor = properties.property("backgroundColor");
    if (backgroundColor.isValid()) {
        QScriptValue red = backgroundColor.property("red");
        QScriptValue green = backgroundColor.property("green");
        QScriptValue blue = backgroundColor.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            _backgroundColor.red = red.toVariant().toInt();
            _backgroundColor.green = green.toVariant().toInt();
            _backgroundColor.blue = blue.toVariant().toInt();
        }
    }

    if (properties.property("backgroundAlpha").isValid()) {
        _backgroundAlpha = properties.property("backgroundAlpha").toVariant().toFloat();
    }
    _qmlElement->setbackgroundColor(toQmlColor(vec4(toGlm(_backgroundColor), _backgroundAlpha)));

    if (properties.property("leftMargin").isValid()) {
        setLeftMargin(properties.property("leftMargin").toVariant().toInt());
    }

    if (properties.property("topMargin").isValid()) {
        setTopMargin(properties.property("topMargin").toVariant().toInt());
    }
}

TextOverlay* TextOverlay::createClone() const {
    return new TextOverlay(this);
}

QScriptValue TextOverlay::getProperty(const QString& property) {
    if (property == "font") {
        QScriptValue font = _scriptEngine->newObject();
        font.setProperty("size", _fontSize);
        return font;
    }
    if (property == "text") {
        return _text;
    }
    if (property == "backgroundColor") {
        return xColorToScriptValue(_scriptEngine, _backgroundColor);
    }
    if (property == "backgroundAlpha") {
        return _backgroundAlpha;
    }
    if (property == "leftMargin") {
        return _leftMargin;
    }
    if (property == "topMargin") {
        return _topMargin;
    }

    return Overlay2D::getProperty(property);
}

QSizeF TextOverlay::textSize(const QString& text) const {
    int lines = 1;
    foreach(QChar c, text) {
        if (c == QChar('\n')) {
            ++lines;
        }
    }
    QFont font(_qmlElement->fontFamily());
    font.setPixelSize(_qmlElement->fontSize());
    QFontMetrics fm(font);
    QSizeF result = QSizeF(fm.width(text), _qmlElement->lineHeight() * lines);
    return result; 
}

void TextOverlay::setFontSize(int fontSize) {
    _fontSize = fontSize;
    _qmlElement->setfontSize(fontSize);
}

void TextOverlay::setText(const QString& text) {
    _text = text;
    _qmlElement->settext(text);
}

void TextOverlay::setLeftMargin(int margin) {
    _leftMargin = margin;
    _qmlElement->setleftMargin(margin);
}

void TextOverlay::setTopMargin(int margin) {
    _topMargin = margin;
    _qmlElement->settopMargin(margin);
}

#include "TextOverlay.moc"
