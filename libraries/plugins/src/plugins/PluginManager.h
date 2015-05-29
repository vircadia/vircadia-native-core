#pragma once

#include "Plugin.h"
#include <QList>
#include <QSharedPointer>

class PluginManager : public QObject {
public:
  static PluginManager * getInstance();
};
