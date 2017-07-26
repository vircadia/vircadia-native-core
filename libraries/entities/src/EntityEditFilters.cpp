//
//  EntityEditFilters.cpp
//  libraries/entities/src
//
//  Created by David Kelly on 2/7/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <QUrl>

#include <ResourceManager.h>
#include "EntityEditFilters.h"

QList<EntityItemID> EntityEditFilters::getZonesByPosition(glm::vec3& position) {
    QList<EntityItemID> zones;
    QList<EntityItemID> missingZones;
    _lock.lockForRead();
    auto zoneIDs = _filterDataMap.keys();
    _lock.unlock();
    for (auto id : zoneIDs) {
        if (!id.isInvalidID()) {
            // for now, look it up in the tree (soon we need to cache or similar?)
            EntityItemPointer itemPtr = _tree->findEntityByEntityItemID(id); 
            auto zone = std::dynamic_pointer_cast<ZoneEntityItem>(itemPtr);
            if (!zone) {
                // TODO: maybe remove later?
                removeFilter(id);
            } else if (zone->contains(position)) {
                zones.append(id);
            }
        } else {
            // the null id is the global filter we put in the domain server's 
            // advanced entity server settings
            zones.append(id);
        }
    }
    return zones;
}

bool EntityEditFilters::filter(glm::vec3& position, EntityItemProperties& propertiesIn, EntityItemProperties& propertiesOut, bool& wasChanged, 
        EntityTree::FilterType filterType, EntityItemID& itemID) {
    
    // get the ids of all the zones (plus the global entity edit filter) that the position
    // lies within
    auto zoneIDs = getZonesByPosition(position);
    for (auto id : zoneIDs) {
        if (!itemID.isInvalidID() && id == itemID) {
            continue;
        }
        
        // get the filter pair, etc...  
        _lock.lockForRead();
        FilterData filterData = _filterDataMap.value(id);
        _lock.unlock();
    
        if (filterData.valid()) {
            if (filterData.rejectAll) {
                return false;
            }
            auto oldProperties = propertiesIn.getDesiredProperties();
            auto specifiedProperties = propertiesIn.getChangedProperties();
            propertiesIn.setDesiredProperties(specifiedProperties);
            QScriptValue inputValues = propertiesIn.copyToScriptValue(filterData.engine, false, true, true);
            propertiesIn.setDesiredProperties(oldProperties);

            auto in = QJsonValue::fromVariant(inputValues.toVariant()); // grab json copy now, because the inputValues might be side effected by the filter.
            QScriptValueList args;
            args << inputValues;
            args << filterType;

            QScriptValue result = filterData.filterFn.call(_nullObjectForFilter, args);
            if (filterData.uncaughtExceptions()) {
                return false;
            }

            if (result.isObject()){
                // make propertiesIn reflect the changes, for next filter...
                propertiesIn.copyFromScriptValue(result, false);

                // and update propertiesOut too.  TODO: this could be more efficient...
                propertiesOut.copyFromScriptValue(result, false);
                // Javascript objects are == only if they are the same object. To compare arbitrary values, we need to use JSON.
                auto out = QJsonValue::fromVariant(result.toVariant());
                wasChanged |= (in != out);
            } else {
                return false;
            }
        }
    }
    // if we made it here, 
    return true;
}

void EntityEditFilters::removeFilter(EntityItemID entityID) {
    QWriteLocker writeLock(&_lock);
    FilterData filterData = _filterDataMap.value(entityID);
    if (filterData.valid()) {
        delete filterData.engine;
    }
    _filterDataMap.remove(entityID);
}

void EntityEditFilters::addFilter(EntityItemID entityID, QString filterURL) {

    QUrl scriptURL(filterURL);
    
    // setting it to an empty string is same as removing 
    if (filterURL.size() == 0) {
        removeFilter(entityID);
        return;
    }
   
    // The following should be abstracted out for use in Agent.cpp (and maybe later AvatarMixer.cpp)
    if (scriptURL.scheme().isEmpty() || (scriptURL.scheme() == URL_SCHEME_FILE)) {
        qWarning() << "Cannot load script from local filesystem, because assignment may be on a different computer.";
        scriptRequestFinished(entityID);
        return;
    }
   
    // first remove any existing info for this entity
    removeFilter(entityID);
    
    // reject all edits until we load the script
    FilterData filterData;
    filterData.rejectAll = true;

    _lock.lockForWrite();
    _filterDataMap.insert(entityID, filterData);
    _lock.unlock();
   
    auto scriptRequest = DependencyManager::get<ResourceManager>()->createResourceRequest(this, scriptURL);
    if (!scriptRequest) {
        qWarning() << "Could not create ResourceRequest for Entity Edit filter script at" << scriptURL.toString();
        scriptRequestFinished(entityID);
        return;
    }
    // Agent.cpp sets up a timeout here, but that is unnecessary, as ResourceRequest has its own.
    connect(scriptRequest, &ResourceRequest::finished, this, [this, entityID]{ EntityEditFilters::scriptRequestFinished(entityID);} );
    // FIXME: handle atp rquests setup here. See Agent::requestScript()
    qInfo() << "Requesting script at URL" << qPrintable(scriptRequest->getUrl().toString());
    scriptRequest->send();
    qDebug() << "script request sent for entity " << entityID;
}

// Copied from ScriptEngine.cpp. We should make this a class method for reuse.
// Note: I've deliberately stopped short of using ScriptEngine instead of QScriptEngine, as that is out of project scope at this point.
static bool hasCorrectSyntax(const QScriptProgram& program) {
    const auto syntaxCheck = QScriptEngine::checkSyntax(program.sourceCode());
    if (syntaxCheck.state() != QScriptSyntaxCheckResult::Valid) {
        const auto error = syntaxCheck.errorMessage();
        const auto line = QString::number(syntaxCheck.errorLineNumber());
        const auto column = QString::number(syntaxCheck.errorColumnNumber());
        const auto message = QString("[SyntaxError] %1 in %2:%3(%4)").arg(error, program.fileName(), line, column);
        qCritical() << qPrintable(message);
        return false;
    }
    return true;
}
static bool hadUncaughtExceptions(QScriptEngine& engine, const QString& fileName) {
    if (engine.hasUncaughtException()) {
        const auto backtrace = engine.uncaughtExceptionBacktrace();
        const auto exception = engine.uncaughtException().toString();
        const auto line = QString::number(engine.uncaughtExceptionLineNumber());
        engine.clearExceptions();

        static const QString SCRIPT_EXCEPTION_FORMAT = "[UncaughtException] %1 in %2:%3";
        auto message = QString(SCRIPT_EXCEPTION_FORMAT).arg(exception, fileName, line);
        if (!backtrace.empty()) {
            static const auto lineSeparator = "\n    ";
            message += QString("\n[Backtrace]%1%2").arg(lineSeparator, backtrace.join(lineSeparator));
        }
        qCritical() << qPrintable(message);
        return true;
    }
    return false;
}

void EntityEditFilters::scriptRequestFinished(EntityItemID entityID) {
    qDebug() << "script request completed for entity " << entityID;
    auto scriptRequest = qobject_cast<ResourceRequest*>(sender());
    const QString urlString = scriptRequest->getUrl().toString();
    if (scriptRequest && scriptRequest->getResult() == ResourceRequest::Success) {
        auto scriptContents = scriptRequest->getData();
        qInfo() << "Downloaded script:" << scriptContents;
        QScriptProgram program(scriptContents, urlString);
        if (hasCorrectSyntax(program)) {
            // create a QScriptEngine for this script
            QScriptEngine* engine = new QScriptEngine();
            engine->evaluate(scriptContents);
            if (!hadUncaughtExceptions(*engine, urlString)) {
                // put the engine in the engine map (so we don't leak them, etc...)
                FilterData filterData;
                filterData.engine = engine;
                filterData.rejectAll = false;
                
                // define the uncaughtException function
                QScriptEngine& engineRef = *engine;
                filterData.uncaughtExceptions = [this, &engineRef, urlString]() { return hadUncaughtExceptions(engineRef, urlString); };

                // now get the filter function
                auto global = engine->globalObject();
                auto entitiesObject = engine->newObject();
                entitiesObject.setProperty("ADD_FILTER_TYPE", EntityTree::FilterType::Add);
                entitiesObject.setProperty("EDIT_FILTER_TYPE", EntityTree::FilterType::Edit);
                entitiesObject.setProperty("PHYSICS_FILTER_TYPE", EntityTree::FilterType::Physics);
                global.setProperty("Entities", entitiesObject);
                filterData.filterFn = global.property("filter");
                if (!filterData.filterFn.isFunction()) {
                    qDebug() << "Filter function specified but not found. Will reject all edits for those without lock rights.";
                    delete engine;
                    filterData.rejectAll=true;
                }
               
                
                _lock.lockForWrite();
                _filterDataMap.insert(entityID, filterData);
                _lock.unlock();

                qDebug() << "script request filter processed for entity id " << entityID;
                
                emit filterAdded(entityID, true);
                return;
            }
        } 
    } else if (scriptRequest) {
        qCritical() << "Failed to download script at" << urlString;
        // See HTTPResourceRequest::onRequestFinished for interpretation of codes. For example, a 404 is code 6 and 403 is 3. A timeout is 2. Go figure.
        qCritical() << "ResourceRequest error was" << scriptRequest->getResult();
    } else {
        qCritical() << "Failed to create script request.";
    }
    emit filterAdded(entityID, false);
}
