
#include "SoundEffect.h"

#include <RegisteredMetaTypes.h>
#include <AudioInjector.h>

SoundEffect::~SoundEffect() {
    if (_injector) {
         // stop will cause the AudioInjector to delete itself.
        _injector->stop();
    }
}

QUrl SoundEffect::getSource() const {
    return _url;
}

void SoundEffect::setSource(QUrl url) {
    _url = url;
    _sound = DependencyManager::get<SoundCache>()->getSound(_url);
}

float SoundEffect::getVolume() const {
    return _volume;
}

void SoundEffect::setVolume(float volume) {
    _volume = volume;
}

void SoundEffect::play(QVariant position) {
    AudioInjectorOptions options;
    options.position = vec3FromVariant(position);
    options.localOnly = true;
    options.volume = _volume;
    if (_injector) {
        _injector->setOptions(options);
        _injector->restart();
    } else {
        QByteArray samples = _sound->getByteArray();
        _injector = AudioInjector::playSound(samples, options);
    }
}
