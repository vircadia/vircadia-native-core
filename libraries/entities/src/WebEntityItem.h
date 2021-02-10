//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebEntityItem_h
#define hifi_WebEntityItem_h

#include "EntityItem.h"

#include "PulsePropertyGroup.h"

class WebEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    WebEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    /// set dimensions in domain scale units (0.0 - 1.0) this will also reset radius appropriately
    virtual void setUnscaledDimensions(const glm::vec3& value) override;
    virtual ShapeType getShapeType() const override { return SHAPE_TYPE_BOX; }

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    virtual bool setSubClassProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    glm::u8vec3 getColor() const;
    void setColor(const glm::u8vec3& value);

    float getAlpha() const;
    void setAlpha(float alpha);

    static const QString DEFAULT_SOURCE_URL;
    void setSourceUrl(const QString& value);
    QString getSourceUrl() const;

    void setDPI(uint16_t value);
    uint16_t getDPI() const;

    void setScriptURL(const QString& value);
    QString getScriptURL() const;

    bool getLocalSafeContext() const;

    static const uint8_t DEFAULT_MAX_FPS;
    void setMaxFPS(uint8_t value);
    uint8_t getMaxFPS() const;

    void setInputMode(const WebInputMode& value);
    WebInputMode getInputMode() const;

    bool getShowKeyboardFocusHighlight() const;
    void setShowKeyboardFocusHighlight(bool value);
    
    bool getUseBackground() const;
    void setUseBackground(bool value);
    
    static const QString DEFAULT_USER_AGENT;
    QString getUserAgent() const;
    void setUserAgent(const QString& value);

    PulsePropertyGroup getPulseProperties() const;

protected:
    glm::u8vec3 _color;
    float _alpha { 1.0f };
    PulsePropertyGroup _pulseProperties;

    QString _sourceUrl;
    uint16_t _dpi;
    QString _scriptURL;
    uint8_t _maxFPS;
    WebInputMode _inputMode;
    bool _showKeyboardFocusHighlight;
    bool _useBackground;
    QString _userAgent;
    bool _localSafeContext { false };
};

#endif // hifi_WebEntityItem_h
