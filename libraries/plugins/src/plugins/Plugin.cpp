#include "Plugin.h"

PluginContainer* Plugin::CONTAINER{ nullptr };

void Plugin::setContainer(PluginContainer* container) {
    CONTAINER = container;
}

bool Plugin::isSupported() const { return true; }

void Plugin::init() {}

void Plugin::deinit() {}

void Plugin::idle() {}
