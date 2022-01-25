//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WebEntityItem.h"

#include <glm/gtx/transform.hpp>

#include <QDebug>
#include <QJsonDocument>

#include <ByteCountCoding.h>
#include <GeometryUtil.h>
#include <shared/LocalFileAccessGate.h>
#include <NetworkingConstants.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

const QString WebEntityItem::DEFAULT_SOURCE_URL = NetworkingConstants::WEB_ENTITY_DEFAULT_SOURCE_URL;
const QString WebEntityItem::DEFAULT_USER_AGENT = NetworkingConstants::WEB_ENTITY_DEFAULT_USER_AGENT;
const uint8_t WebEntityItem::DEFAULT_MAX_FPS = 10;

EntityItemPointer WebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    std::shared_ptr<WebEntityItem> entity(new WebEntityItem(entityID), [](WebEntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

WebEntityItem::WebEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID) {
    // this initialzation of localSafeContext is reading a thread-local variable and that is depends on
    // the ctor being executed on the same thread as the script, assuming it's being create by a script
    _localSafeContext = hifi::scripting::isLocalAccessSafeThread();
    _type = EntityTypes::Web;
}

void WebEntityItem::setUnscaledDimensions(const glm::vec3& value) {
    // NOTE: Web Entities always have a "depth" of 1cm.
    const float WEB_ENTITY_ITEM_FIXED_DEPTH = 0.01f;
    EntityItem::setUnscaledDimensions(glm::vec3(value.x, value.y, WEB_ENTITY_ITEM_FIXED_DEPTH));
}

EntityItemProperties WebEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(alpha, getAlpha);
    withReadLock([&] {
        _pulseProperties.getProperties(properties);
    });

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(sourceUrl, getSourceUrl);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dpi, getDPI);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(scriptURL, getScriptURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(maxFPS, getMaxFPS);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(inputMode, getInputMode);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(showKeyboardFocusHighlight, getShowKeyboardFocusHighlight);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(useBackground, getUseBackground);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(userAgent, getUserAgent);
    return properties;
}

bool WebEntityItem::setSubClassProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alpha, setAlpha);
    withWriteLock([&] {
        bool pulsePropertiesChanged = _pulseProperties.setProperties(properties);
        somethingChanged |= pulsePropertiesChanged;
        _needsRenderUpdate |= pulsePropertiesChanged;
    });

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(sourceUrl, setSourceUrl);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dpi, setDPI);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(scriptURL, setScriptURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(maxFPS, setMaxFPS);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(inputMode, setInputMode);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(showKeyboardFocusHighlight, setShowKeyboardFocusHighlight);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(useBackground, setUseBackground);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(userAgent, setUserAgent);

    return somethingChanged;
}

int WebEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, glm::u8vec3, setColor);
    READ_ENTITY_PROPERTY(PROP_ALPHA, float, setAlpha);
    withWriteLock([&] {
        int bytesFromPulse = _pulseProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData,
            somethingChanged);
        bytesRead += bytesFromPulse;
        dataAt += bytesFromPulse;
    });

    READ_ENTITY_PROPERTY(PROP_SOURCE_URL, QString, setSourceUrl);
    READ_ENTITY_PROPERTY(PROP_DPI, uint16_t, setDPI);
    READ_ENTITY_PROPERTY(PROP_SCRIPT_URL, QString, setScriptURL);
    READ_ENTITY_PROPERTY(PROP_MAX_FPS, uint8_t, setMaxFPS);
    READ_ENTITY_PROPERTY(PROP_INPUT_MODE, WebInputMode, setInputMode);
    READ_ENTITY_PROPERTY(PROP_SHOW_KEYBOARD_FOCUS_HIGHLIGHT, bool, setShowKeyboardFocusHighlight);
    READ_ENTITY_PROPERTY(PROP_WEB_USE_BACKGROUND, bool, setUseBackground);
    READ_ENTITY_PROPERTY(PROP_USER_AGENT, QString, setUserAgent);

    return bytesRead;
}

EntityPropertyFlags WebEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_ALPHA;
    requestedProperties += _pulseProperties.getEntityProperties(params);

    requestedProperties += PROP_SOURCE_URL;
    requestedProperties += PROP_DPI;
    requestedProperties += PROP_SCRIPT_URL;
    requestedProperties += PROP_MAX_FPS;
    requestedProperties += PROP_INPUT_MODE;
    requestedProperties += PROP_SHOW_KEYBOARD_FOCUS_HIGHLIGHT;
    requestedProperties += PROP_WEB_USE_BACKGROUND;
    requestedProperties += PROP_USER_AGENT;
    return requestedProperties;
}

void WebEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA, getAlpha());
    withReadLock([&] {
        _pulseProperties.appendSubclassData(packetData, params, entityTreeElementExtraEncodeData, requestedProperties,
            propertyFlags, propertiesDidntFit, propertyCount, appendState);
    });

    APPEND_ENTITY_PROPERTY(PROP_SOURCE_URL, getSourceUrl());
    APPEND_ENTITY_PROPERTY(PROP_DPI, getDPI());
    APPEND_ENTITY_PROPERTY(PROP_SCRIPT_URL, getScriptURL());
    APPEND_ENTITY_PROPERTY(PROP_MAX_FPS, getMaxFPS());
    APPEND_ENTITY_PROPERTY(PROP_INPUT_MODE, (uint32_t)getInputMode());
    APPEND_ENTITY_PROPERTY(PROP_SHOW_KEYBOARD_FOCUS_HIGHLIGHT, getShowKeyboardFocusHighlight());
    APPEND_ENTITY_PROPERTY(PROP_WEB_USE_BACKGROUND, getUseBackground());
    APPEND_ENTITY_PROPERTY(PROP_USER_AGENT, getUserAgent());
}

void WebEntityItem::setColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _color != value;
        _color = value;
    });
}

glm::u8vec3 WebEntityItem::getColor() const {
    return resultWithReadLock<glm::u8vec3>([&] {
        return _color;
    });
}

bool WebEntityItem::getLocalSafeContext() const {
    return resultWithReadLock<bool>([&] {
        return _localSafeContext;
    });
}

void WebEntityItem::setAlpha(float alpha) {
    withWriteLock([&] {
        _needsRenderUpdate |= _alpha != alpha;
        _alpha = alpha;
    });
}

float WebEntityItem::getAlpha() const {
    return resultWithReadLock<float>([&] {
        return _alpha;
    });
}

void WebEntityItem::setSourceUrl(const QString& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _sourceUrl != value;
        _sourceUrl = value;
    });
}

QString WebEntityItem::getSourceUrl() const { 
    return resultWithReadLock<QString>([&] {
        return _sourceUrl;
    });
}

void WebEntityItem::setDPI(uint16_t value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _dpi != value;
        _dpi = value;
    });
}

uint16_t WebEntityItem::getDPI() const {
    return resultWithReadLock<uint16_t>([&] {
        return _dpi;
    });
}

void WebEntityItem::setScriptURL(const QString& value) {
    auto newURL = QUrl::fromUserInput(value);

    if (!newURL.isValid()) {
        qCDebug(entities) << "Not setting web entity script URL since" << value << "cannot be parsed to a valid URL.";
        return;
    }

    auto urlString = newURL.toDisplayString();

    withWriteLock([&] {
        _needsRenderUpdate |= _scriptURL != urlString;
        _scriptURL = urlString;
    });
}

QString WebEntityItem::getScriptURL() const {
    return resultWithReadLock<QString>([&] {
        return _scriptURL;
    });
}

void WebEntityItem::setMaxFPS(uint8_t value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _maxFPS != value;
        _maxFPS = value;
    });
}

uint8_t WebEntityItem::getMaxFPS() const {
    return resultWithReadLock<uint8_t>([&] {
        return _maxFPS;
    });
}

void WebEntityItem::setInputMode(const WebInputMode& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _inputMode != value;
        _inputMode = value;
    });
}

WebInputMode WebEntityItem::getInputMode() const {
    return resultWithReadLock<WebInputMode>([&] {
        return _inputMode;
    });
}

void WebEntityItem::setShowKeyboardFocusHighlight(bool value) {
    _showKeyboardFocusHighlight = value;
}

bool WebEntityItem::getShowKeyboardFocusHighlight() const {
    return _showKeyboardFocusHighlight;
}

void WebEntityItem::setUseBackground(bool value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _useBackground != value;
        _useBackground = value;
    });
}

bool WebEntityItem::getUseBackground() const {
    return resultWithReadLock<bool>([&] { return _useBackground; });
}

void WebEntityItem::setUserAgent(const QString& value) {
    withWriteLock([&] {
        _needsRenderUpdate |= _userAgent != value;
        _userAgent = value;
    });
}

QString WebEntityItem::getUserAgent() const {
    return resultWithReadLock<QString>([&] { return _userAgent; });
}

PulsePropertyGroup WebEntityItem::getPulseProperties() const {
    return resultWithReadLock<PulsePropertyGroup>([&] {
        return _pulseProperties;
    });
}
