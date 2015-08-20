#pragma once

#include "Plugin.h"
#include <QList>
#include <QVector>
#include <QSharedPointer>

class DisplayPlugin;
class InputPlugin;

using DisplayPluginPointer = QSharedPointer<DisplayPlugin>;
using DisplayPluginList = QVector<DisplayPluginPointer>;
using InputPluginPointer = QSharedPointer<InputPlugin>;
using InputPluginList = QVector<InputPluginPointer>;

class PluginManager : public QObject {
public:
  static PluginManager* getInstance();

  const DisplayPluginList& getDisplayPlugins();
  const InputPluginList& getInputPlugins();
};
