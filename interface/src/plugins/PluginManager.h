#pragma once

#include "plugins/Plugin.h"
#include "plugins/display/DisplayPlugin.h"
#include <QList>
#include <QSharedPointer>

class PluginManager : public QObject {
public:
  static PluginManager * getInstance();
  const QList<QSharedPointer<DisplayPlugin>> getDisplayPlugins();
};
