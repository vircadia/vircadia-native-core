
#include "SoundEffect.h"

#include <RegisteredMetaTypes.h>

SoundEffect::~SoundEffect() {
    if (_injector) {
        DependencyManager::get<AudioInjectorManager>()->stop(_injector);
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

void SoundEffect::play(const QVariant& position) {
    AudioInjectorOptions options;
    options.position = vec3FromVariant(position);
    options.localOnly = true;
    options.volume = _volume;
    if (_injector) {
        DependencyManager::get<AudioInjectorManager>()->setOptionsAndRestart(_injector, options);
    } else {
        _injector = DependencyManager::get<AudioInjectorManager>()->playSound(_sound, options);
    }
}
