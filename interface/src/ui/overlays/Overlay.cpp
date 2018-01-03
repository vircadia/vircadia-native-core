//
//  Overlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Overlay.h"

#include <NumericalConstants.h>
#include <RegisteredMetaTypes.h>

#include "Application.h"

const xColor Overlay::DEFAULT_OVERLAY_COLOR = { 255, 255, 255 };
const float Overlay::DEFAULT_ALPHA = 0.7f;

Overlay::Overlay() :
    _renderItemID(render::Item::INVALID_ITEM_ID),
    _isLoaded(true),
    _alpha(DEFAULT_ALPHA),
    _pulse(1.0f),
    _pulseMax(0.0f),
    _pulseMin(0.0f),
    _pulsePeriod(1.0f),
    _pulseDirection(1.0f),
    _lastPulseUpdate(usecTimestampNow()),
    _alphaPulse(0.0f),
    _colorPulse(0.0f),
    _color(DEFAULT_OVERLAY_COLOR),
    _visible(true),
    _anchor(NO_ANCHOR)
{
}

Overlay::Overlay(const Overlay* overlay) :
    _renderItemID(render::Item::INVALID_ITEM_ID),
    _isLoaded(overlay->_isLoaded),
    _alpha(overlay->_alpha),
    _pulse(overlay->_pulse),
    _pulseMax(overlay->_pulseMax),
    _pulseMin(overlay->_pulseMin),
    _pulsePeriod(overlay->_pulsePeriod),
    _pulseDirection(overlay->_pulseDirection),
    _lastPulseUpdate(usecTimestampNow()),
    _alphaPulse(overlay->_alphaPulse),
    _colorPulse(overlay->_colorPulse),
    _color(overlay->_color),
    _visible(overlay->_visible),
    _anchor(overlay->_anchor)
{
}

Overlay::~Overlay() {
}

void Overlay::setProperties(const QVariantMap& properties) {
    bool valid;
    auto color = xColorFromVariant(properties["color"], valid);
    if (valid) {
        _color = color;
    }

    if (properties["alpha"].isValid()) {
        setAlpha(properties["alpha"].toFloat());
    }

    if (properties["pulseMax"].isValid()) {
        setPulseMax(properties["pulseMax"].toFloat());
    }

    if (properties["pulseMin"].isValid()) {
        setPulseMin(properties["pulseMin"].toFloat());
    }

    if (properties["pulsePeriod"].isValid()) {
        setPulsePeriod(properties["pulsePeriod"].toFloat());
    }

    if (properties["alphaPulse"].isValid()) {
        setAlphaPulse(properties["alphaPulse"].toFloat());
    }

    if (properties["colorPulse"].isValid()) {
        setColorPulse(properties["colorPulse"].toFloat());
    }

    if (properties["visible"].isValid()) {
        bool visible = properties["visible"].toBool();
        setVisible(visible);
    }

    if (properties["anchor"].isValid()) {
        QString property = properties["anchor"].toString();
        if (property == "MyAvatar") {
            setAnchor(MY_AVATAR);
        }
    }
}

// JSDoc for copying to @typedefs of overlay types that inherit Overlay.
/**jsdoc
  * @property {string} type=TODO - Has the value <code>"TODO"</code>. <em>Read-only.</em>
  * @property {Color} color=255,255,255 - The color of the overlay.
  * @property {number} alpha=0.7 - The opacity of the overlay, <code>0.0</code> - <code>1.0</code>.
  * @property {number} pulseMax=0 - The maximum value of the pulse multiplier.
  * @property {number} pulseMin=0 - The minimum value of the pulse multiplier.
  * @property {number} pulsePeriod=1 - The duration of the color and alpha pulse, in seconds. A pulse multiplier value goes from 
  *     <code>pulseMin</code> to <code>pulseMax</code>, then <code>pulseMax</code> to <code>pulseMin</code> in one period.
  * @property {number} alphaPulse=0 - If non-zero, the alpha of the overlay is pulsed: the alpha value is multiplied by the 
  *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0 
  *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
  *     used.)
  * @property {number} colorPulse=0 - If non-zero, the color of the overlay is pulsed: the color value is multiplied by the 
  *     current pulse multiplier value each frame. If > 0 the pulse multiplier is applied in phase with the pulse period; if < 0 
  *     the pulse multiplier is applied out of phase with the pulse period. (The magnitude of the property isn't otherwise
  *     used.)
  * @property {boolean} visible=true - If <code>true</code>, the overlay is rendered, otherwise it is not rendered.
  * @property {string} anchor="" - If set to <code>"MyAvatar"</code> then the overlay is attached to your avatar, moving and
  *     rotating as you move your avatar.
  */
QVariant Overlay::getProperty(const QString& property) {
    if (property == "type") {
        return QVariant(getType());
    }
    if (property == "color") {
        return xColorToVariant(_color);
    }
    if (property == "alpha") {
        return _alpha;
    }
    if (property == "pulseMax") {
        return _pulseMax;
    }
    if (property == "pulseMin") {
        return _pulseMin;
    }
    if (property == "pulsePeriod") {
        return _pulsePeriod;
    }
    if (property == "alphaPulse") {
        return _alphaPulse;
    }
    if (property == "colorPulse") {
        return _colorPulse;
    }
    if (property == "visible") {
        return _visible;
    }
    if (property == "anchor") {
        return _anchor == MY_AVATAR ? "MyAvatar" : "";
    }

    return QVariant();
}

xColor Overlay::getColor() { 
    if (_colorPulse == 0.0f) {
        return _color; 
    }

    float pulseLevel = updatePulse();
    xColor result = _color;
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

float Overlay::getAlpha() {
    if (_alphaPulse == 0.0f) {
        return _alpha; 
    }
    float pulseLevel = updatePulse();
    return (_alphaPulse >= 0.0f) ? _alpha * pulseLevel : _alpha * (1.0f - pulseLevel);
}

// pulse travels from min to max, then max to min in one period.
float Overlay::updatePulse() {
    if (_pulsePeriod <= 0.0f) {
        return _pulse;
    }
    quint64 now = usecTimestampNow();
    quint64 elapsedUSecs = (now - _lastPulseUpdate);
    float elapsedSeconds =  (float)elapsedUSecs / (float)USECS_PER_SECOND;
    float elapsedPeriods = elapsedSeconds / _pulsePeriod;

    // we can safely remove any "full" periods, since those just rotate us back
    // to our final pulse level
    elapsedPeriods = fmod(elapsedPeriods, 1.0f);
    _lastPulseUpdate = now;

    float pulseDistance =  (_pulseMax - _pulseMin);
    float pulseDistancePerPeriod = pulseDistance * 2.0f;

    float pulseDelta = _pulseDirection * pulseDistancePerPeriod * elapsedPeriods;
    float newPulse = _pulse + pulseDelta;
    float limit = (_pulseDirection > 0.0f) ? _pulseMax : _pulseMin;
    float passedLimit = (_pulseDirection > 0.0f) ? (newPulse >= limit) : (newPulse <= limit);

    if (passedLimit) {
        float pulseDeltaToLimit = newPulse - limit;
        float pulseDeltaFromLimitBack = pulseDelta - pulseDeltaToLimit;
        pulseDelta = -pulseDeltaFromLimitBack;
        _pulseDirection *= -1.0f;
    }
    _pulse += pulseDelta;

    return _pulse;
}

bool Overlay::addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    _renderItemID = scene->allocateID();
    transaction.resetItem(_renderItemID, std::make_shared<Overlay::Payload>(overlay));
    return true;
}

void Overlay::removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    transaction.removeItem(_renderItemID);
    render::Item::clearID(_renderItemID);
}

QScriptValue OverlayIDtoScriptValue(QScriptEngine* engine, const OverlayID& id) {
    return quuidToScriptValue(engine, id);
}

void OverlayIDfromScriptValue(const QScriptValue &object, OverlayID& id) {
    quuidFromScriptValue(object, id);
}

QVector<OverlayID> qVectorOverlayIDFromScriptValue(const QScriptValue& array) {
    if (!array.isArray()) {
        return QVector<OverlayID>();
    }
    QVector<OverlayID> newVector;
    int length = array.property("length").toInteger();
    newVector.reserve(length);
    for (int i = 0; i < length; i++) {
        newVector << OverlayID(array.property(i).toString());
    }
    return newVector;
}
