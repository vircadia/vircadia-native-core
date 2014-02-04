//
//  MetavoxelUtil.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/30/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelUtil__
#define __interface__MetavoxelUtil__

#include <QUrl>
#include <QUuid>
#include <QWidget>

#include "Bitstream.h"

class QByteArray;
class QLineEdit;

class HifiSockAddr;

/// Reads and returns the session ID from a datagram.
/// \param[out] headerPlusIDSize the size of the header (including the session ID) within the data
/// \return the session ID, or a null ID if invalid (in which case a warning will be logged)
QUuid readSessionID(const QByteArray& data, const HifiSockAddr& sender, int& headerPlusIDSize);

/// A streamable axis-aligned bounding box.
class Box {
    STREAMABLE

public:
    
    STREAM glm::vec3 minimum;
    STREAM glm::vec3 maximum;
    
    bool contains(const Box& other) const;
};

DECLARE_STREAMABLE_METATYPE(Box)

/// Combines a URL with a set of typed parameters.
class ParameterizedURL {
public:
    
    ParameterizedURL(const QUrl& url = QUrl(), const QVariantHash& parameters = QVariantHash());
    
    bool isValid() const { return _url.isValid(); }
    
    void setURL(const QUrl& url) { _url = url; }
    const QUrl& getURL() const { return _url; }
    
    void setParameters(const QVariantHash& parameters) { _parameters = parameters; }
    const QVariantHash& getParameters() const { return _parameters; }
    
    bool operator==(const ParameterizedURL& other) const;
    bool operator!=(const ParameterizedURL& other) const;
    
private:
    
    QUrl _url;
    QVariantHash _parameters;
};

uint qHash(const ParameterizedURL& url, uint seed = 0);

Bitstream& operator<<(Bitstream& out, const ParameterizedURL& url);
Bitstream& operator>>(Bitstream& in, ParameterizedURL& url);

Q_DECLARE_METATYPE(ParameterizedURL)

/// Allows editing parameterized URLs.
class ParameterizedURLEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(ParameterizedURL url MEMBER _url WRITE setURL NOTIFY urlChanged USER true)

public:
    
    ParameterizedURLEditor(QWidget* parent = NULL);

signals:

    void urlChanged(const ParameterizedURL& url);

public slots:

    void setURL(const ParameterizedURL& url);

private slots:

    void updateURL();    
    
private:
    
    ParameterizedURL _url;
    QLineEdit* _line;
};

#endif /* defined(__interface__MetavoxelUtil__) */
