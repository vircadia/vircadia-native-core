//
//  TextureCache.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QNetworkReply>
#include <QOpenGLFramebufferObject>

#include <glm/gtc/random.hpp>

#include "Application.h"
#include "TextureCache.h"

TextureCache::TextureCache() :
    _permutationNormalTextureID(0),
    _whiteTextureID(0),
    _blueTextureID(0),
    _primaryFramebufferObject(NULL),
    _secondaryFramebufferObject(NULL),
    _tertiaryFramebufferObject(NULL)
{
}

TextureCache::~TextureCache() {
    if (_permutationNormalTextureID != 0) {
        glDeleteTextures(1, &_permutationNormalTextureID);
    }
    if (_whiteTextureID != 0) {
        glDeleteTextures(1, &_whiteTextureID);
    }
    foreach (GLuint id, _fileTextureIDs) {
        glDeleteTextures(1, &id);
    }
    if (_primaryFramebufferObject != NULL) {
        delete _primaryFramebufferObject;
        glDeleteTextures(1, &_primaryDepthTextureID);
    }
    if (_secondaryFramebufferObject != NULL) {
        delete _secondaryFramebufferObject;
    }
    if (_tertiaryFramebufferObject != NULL) {
        delete _tertiaryFramebufferObject;
    }
}

GLuint TextureCache::getPermutationNormalTextureID() {
    if (_permutationNormalTextureID == 0) {
        glGenTextures(1, &_permutationNormalTextureID);
        glBindTexture(GL_TEXTURE_2D, _permutationNormalTextureID);
        
        // the first line consists of random permutation offsets
        unsigned char data[256 * 2 * 3];
        for (int i = 0; i < 256 * 3; i++) {
            data[i] = rand() % 256;
        }
        // the next, random unit normals
        for (int i = 256 * 3; i < 256 * 3 * 2; i += 3) {
            glm::vec3 randvec = glm::sphericalRand(1.0f);
            data[i] = ((randvec.x + 1.0f) / 2.0f) * 255.0f;
            data[i + 1] = ((randvec.y + 1.0f) / 2.0f) * 255.0f;
            data[i + 2] = ((randvec.z + 1.0f) / 2.0f) * 255.0f;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return _permutationNormalTextureID;
}

const char OPAQUE_WHITE[] = { 0xFF, 0xFF, 0xFF, 0xFF };
const char OPAQUE_BLUE[] = { 0x80, 0x80, 0xFF, 0xFF };

static void loadSingleColorTexture(const char* color) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

GLuint TextureCache::getWhiteTextureID() {
    if (_whiteTextureID == 0) {
        glGenTextures(1, &_whiteTextureID);
        glBindTexture(GL_TEXTURE_2D, _whiteTextureID);
        loadSingleColorTexture(OPAQUE_WHITE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return _whiteTextureID;
}

GLuint TextureCache::getBlueTextureID() {
    if (_blueTextureID == 0) {
        glGenTextures(1, &_blueTextureID);
        glBindTexture(GL_TEXTURE_2D, _blueTextureID);
        loadSingleColorTexture(OPAQUE_BLUE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return _blueTextureID;
}

GLuint TextureCache::getFileTextureID(const QString& filename) {
    GLuint id = _fileTextureIDs.value(filename);
    if (id == 0) {
        switchToResourcesParentIfRequired();
        QImage image = QImage(filename).convertToFormat(QImage::Format_ARGB32);
    
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 1,
            GL_BGRA, GL_UNSIGNED_BYTE, image.constBits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        _fileTextureIDs.insert(filename, id);
    }
    return id;
}

QSharedPointer<NetworkTexture> TextureCache::getTexture(const QUrl& url, bool normalMap, bool dilatable) {
    QSharedPointer<NetworkTexture> texture;
    if (dilatable) {
        texture = _dilatableNetworkTextures.value(url);
        if (texture.isNull()) {
            texture = QSharedPointer<NetworkTexture>(new DilatableNetworkTexture(url, normalMap));
            _dilatableNetworkTextures.insert(url, texture);
        }
    } else {
        texture = _networkTextures.value(url);
        if (texture.isNull()) {
            texture = QSharedPointer<NetworkTexture>(new NetworkTexture(url, normalMap));
            _networkTextures.insert(url, texture);
        }
    }
    return texture;
}

QOpenGLFramebufferObject* TextureCache::getPrimaryFramebufferObject() {
    if (_primaryFramebufferObject == NULL) {
        _primaryFramebufferObject = createFramebufferObject();
        
        glGenTextures(1, &_primaryDepthTextureID);
        glBindTexture(GL_TEXTURE_2D, _primaryDepthTextureID);
        QSize size = Application::getInstance()->getGLWidget()->size();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.width(), size.height(),
            0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        _primaryFramebufferObject->bind();
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _primaryDepthTextureID, 0);
        _primaryFramebufferObject->release();
    }
    return _primaryFramebufferObject;
}

GLuint TextureCache::getPrimaryDepthTextureID() {
    // ensure that the primary framebuffer object is initialized before returning the depth texture id
    getPrimaryFramebufferObject();
    return _primaryDepthTextureID;
}

QOpenGLFramebufferObject* TextureCache::getSecondaryFramebufferObject() {
    if (_secondaryFramebufferObject == NULL) {
        _secondaryFramebufferObject = createFramebufferObject();
    }
    return _secondaryFramebufferObject;
}

QOpenGLFramebufferObject* TextureCache::getTertiaryFramebufferObject() {
    if (_tertiaryFramebufferObject == NULL) {
        _tertiaryFramebufferObject = createFramebufferObject();
    }
    return _tertiaryFramebufferObject;
}

bool TextureCache::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Resize) {
        QSize size = static_cast<QResizeEvent*>(event)->size();
        if (_primaryFramebufferObject != NULL && _primaryFramebufferObject->size() != size) {
            delete _primaryFramebufferObject;
            _primaryFramebufferObject = NULL;
            glDeleteTextures(1, &_primaryDepthTextureID);
        }
        if (_secondaryFramebufferObject != NULL && _secondaryFramebufferObject->size() != size) {
            delete _secondaryFramebufferObject;
            _secondaryFramebufferObject = NULL;
        }
        if (_tertiaryFramebufferObject != NULL && _tertiaryFramebufferObject->size() != size) {
            delete _tertiaryFramebufferObject;
            _tertiaryFramebufferObject = NULL;
        }
    }
    return false;
}

QOpenGLFramebufferObject* TextureCache::createFramebufferObject() {
    QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(Application::getInstance()->getGLWidget()->size());
    Application::getInstance()->getGLWidget()->installEventFilter(this);
    
    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return fbo;
}

Texture::Texture() {
    glGenTextures(1, &_id);
}

Texture::~Texture() {
    glDeleteTextures(1, &_id);
}

NetworkTexture::NetworkTexture(const QUrl& url, bool normalMap) : _reply(NULL), _averageColor(1.0f, 1.0f, 1.0f, 1.0f) {
    if (!url.isValid()) {
        return;
    }
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    _reply = Application::getInstance()->getNetworkAccessManager()->get(request);
    
    connect(_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleDownloadProgress(qint64,qint64)));
    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleReplyError()));
    
    // default to white/blue
    glBindTexture(GL_TEXTURE_2D, getID());
    loadSingleColorTexture(normalMap ? OPAQUE_BLUE : OPAQUE_WHITE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

NetworkTexture::~NetworkTexture() {
    if (_reply != NULL) {
        delete _reply;
    }
}

void NetworkTexture::imageLoaded(const QImage& image) {
    // nothing by default
}

void NetworkTexture::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesReceived < bytesTotal && !_reply->isFinished()) {
        return;
    }

    QByteArray entirety = _reply->readAll();
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
    
    QImage image = QImage::fromData(entirety).convertToFormat(QImage::Format_ARGB32);
    
    // sum up the colors for the average
    glm::vec4 accumulated;
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            QRgb pixel = image.pixel(x, y);
            accumulated.r += qRed(pixel);
            accumulated.g += qGreen(pixel);
            accumulated.b += qBlue(pixel);
            accumulated.a += qAlpha(pixel);
        }
    }
    const float EIGHT_BIT_MAXIMUM = 255.0f;
    _averageColor = accumulated / (image.width() * image.height() * EIGHT_BIT_MAXIMUM);
    
    imageLoaded(image);
    glBindTexture(GL_TEXTURE_2D, getID());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 1,
        GL_BGRA, GL_UNSIGNED_BYTE, image.constBits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void NetworkTexture::handleReplyError() {
    qDebug() << _reply->errorString() << "\n";
    
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
}

DilatableNetworkTexture::DilatableNetworkTexture(const QUrl& url, bool normalMap) :
    NetworkTexture(url, normalMap),
    _innerRadius(0),
    _outerRadius(0)
{
}

void DilatableNetworkTexture::imageLoaded(const QImage& image) {
    _image = image;
    
    // scan out from the center to find inner and outer radii
    int halfWidth = image.width() / 2;
    int halfHeight = image.height() / 2;
    const int BLACK_THRESHOLD = 32;
    while (_innerRadius < halfWidth && qGray(image.pixel(halfWidth + _innerRadius, halfHeight)) < BLACK_THRESHOLD) {
        _innerRadius++;
    }
    _outerRadius = _innerRadius;
    const int TRANSPARENT_THRESHOLD = 32;
    while (_outerRadius < halfWidth && qAlpha(image.pixel(halfWidth + _outerRadius, halfHeight)) > TRANSPARENT_THRESHOLD) {
        _outerRadius++;
    }
    
    // clear out any textures we generated before loading
    _dilatedTextures.clear();
}

QSharedPointer<Texture> DilatableNetworkTexture::getDilatedTexture(float dilation) {
    QSharedPointer<Texture> texture = _dilatedTextures.value(dilation);
    if (texture.isNull()) {
        texture = QSharedPointer<Texture>(new Texture());
        
        if (!_image.isNull()) {
            QImage dilatedImage = _image;
            QPainter painter;
            painter.begin(&dilatedImage);
            QPainterPath path;
            qreal radius = glm::mix(_innerRadius, _outerRadius, dilation);
            path.addEllipse(QPointF(_image.width() / 2.0, _image.height() / 2.0), radius, radius);
            painter.fillPath(path, Qt::black);
            painter.end();
            
            glBindTexture(GL_TEXTURE_2D, texture->getID());
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dilatedImage.width(), dilatedImage.height(), 1,
                GL_BGRA, GL_UNSIGNED_BYTE, dilatedImage.constBits());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        _dilatedTextures.insert(dilation, texture);
    }
    return texture;
}

