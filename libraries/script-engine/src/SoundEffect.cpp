
#include "SoundEffect.h"

#include <RegisteredMetaTypes.h>
#include <AudioInjector.h>

SoundEffect::~SoundEffect() {
    if (_sound) {
        _sound->deleteLater();
    }
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

void SoundEffect::play(QVariant position) {
    AudioInjectorOptions options;
    options.position = vec3FromVariant(position);
    options.localOnly = true;
    if (_injector) {
        _injector->setOptions(options);
        _injector->restart();
    } else {
        QByteArray samples = _sound->getByteArray();
        _injector = AudioInjector::playSound(samples, options);
    }
}

#include "SoundEffect.moc"
