//
//  TextureCache.h
//  interface
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__TextureCache__
#define __interface__TextureCache__

#include <QHash>
#include <QImage>
#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>

#include "InterfaceConfig.h"

class QNetworkReply;
class QOpenGLFramebufferObject;

class NetworkTexture;

/// Stores cached textures, including render-to-texture targets.
class TextureCache : public QObject {
    Q_OBJECT
    
public:
    
    TextureCache();
    ~TextureCache();
    
    /// Returns the ID of the permutation/normal texture used for Perlin noise shader programs.  This texture
    /// has two lines: the first, a set of random numbers in [0, 255] to be used as permutation offsets, and
    /// the second, a set of random unit vectors to be used as noise gradients.
    GLuint getPermutationNormalTextureID();

    /// Returns the ID of an opaque white texture (useful for a default).
    GLuint getWhiteTextureID();

    /// Returns the ID of a pale blue texture (useful for a normal map).
    GLuint getBlueTextureID();
    
    /// Returns the ID of a texture containing the contents of the specified file, loading it if necessary. 
    GLuint getFileTextureID(const QString& filename);

    /// Loads a texture from the specified URL.
    QSharedPointer<NetworkTexture> getTexture(const QUrl& url, bool normalMap = false, bool dilatable = false);

    /// Returns a pointer to the primary framebuffer object.  This render target includes a depth component, and is
    /// used for scene rendering.
    QOpenGLFramebufferObject* getPrimaryFramebufferObject();
    
    /// Returns the ID of the primary framebuffer object's depth texture.  This contains the Z buffer used in rendering.
    GLuint getPrimaryDepthTextureID();
    
    /// Returns a pointer to the secondary framebuffer object, used as an additional render target when performing full
    /// screen effects.
    QOpenGLFramebufferObject* getSecondaryFramebufferObject();
    
    /// Returns a pointer to the tertiary framebuffer object, used as an additional render target when performing full
    /// screen effects.
    QOpenGLFramebufferObject* getTertiaryFramebufferObject();
    
    virtual bool eventFilter(QObject* watched, QEvent* event);

private:
    
    QOpenGLFramebufferObject* createFramebufferObject();
    
    GLuint _permutationNormalTextureID;
    GLuint _whiteTextureID;
    GLuint _blueTextureID;
    
    QHash<QString, GLuint> _fileTextureIDs;

    QHash<QUrl, QWeakPointer<NetworkTexture> > _networkTextures;
    QHash<QUrl, QWeakPointer<NetworkTexture> > _dilatableNetworkTextures;
    
    GLuint _primaryDepthTextureID;
    QOpenGLFramebufferObject* _primaryFramebufferObject;
    QOpenGLFramebufferObject* _secondaryFramebufferObject;
    QOpenGLFramebufferObject* _tertiaryFramebufferObject;
};

/// A simple object wrapper for an OpenGL texture.
class Texture {
public:
    
    Texture();
    ~Texture();

    GLuint getID() const { return _id; }

private:
    
    GLuint _id;
};

/// A texture loaded from the network.
class NetworkTexture : public QObject, public Texture {
    Q_OBJECT

public:
    
    NetworkTexture(const QUrl& url, bool normalMap);
    ~NetworkTexture();

    /// Returns the average color over the entire texture.
    const glm::vec4& getAverageColor() const { return _averageColor; }

protected:

    virtual void imageLoaded(const QImage& image);    
    
private slots:
    
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleReplyError();    
    
private:
    
    QNetworkReply* _reply;
    glm::vec4 _averageColor;
};

/// Caches derived, dilated textures.
class DilatableNetworkTexture : public NetworkTexture {
    Q_OBJECT
    
public:
    
    DilatableNetworkTexture(const QUrl& url, bool normalMap);
    
    /// Returns a pointer to a texture with the requested amount of dilation.
    QSharedPointer<Texture> getDilatedTexture(float dilation);
    
protected:

    virtual void imageLoaded(const QImage& image);
    
private:
    
    QImage _image;
    int _innerRadius;
    int _outerRadius;
    
    QMap<float, QWeakPointer<Texture> > _dilatedTextures;    
};

#endif /* defined(__interface__TextureCache__) */
