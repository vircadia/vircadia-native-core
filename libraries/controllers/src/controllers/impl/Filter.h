//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filter_h
#define hifi_Controllers_Filter_h

#include <list>
#include <memory>
#include <numeric>
#include <map>

#include <shared/Factory.h>

#include <GLMHelpers.h>

#include <QtCore/QEasingCurve>

#include "../Pose.h"

class QJsonValue;

namespace controller {

    // Encapsulates part of a filter chain
    class Filter {
    public:
        using Pointer = std::shared_ptr<Filter>;
        using List = std::list<Pointer>;
        using Lambda = std::function<float(float)>;
        using Factory = hifi::SimpleFactory<Filter, QString>;

        virtual float apply(float value) const = 0;
        virtual Pose apply(Pose value) const = 0;

        // Factory features
        virtual bool parseParameters(const QJsonValue& parameters) { return true; }

        static Pointer parse(const QJsonValue& json);
        static void registerBuilder(const QString& name, Factory::Builder builder);
        static Factory& getFactory() { return _factory; }

        static bool parseSingleFloatParameter(const QJsonValue& parameters, const QString& name, float& output);
        static bool parseVec3Parameter(const QJsonValue& parameters, glm::vec3& output);
        static bool parseQuatParameter(const QJsonValue& parameters, glm::quat& output);
        static bool parseMat4Parameter(const QJsonValue& parameters, glm::mat4& output);
    protected:
        static Factory _factory;
    };
}

#define REGISTER_FILTER_CLASS(classEntry) \
    private: \
    using Registrar = Filter::Factory::Registrar<classEntry>; \
    static Registrar _registrar;

#define REGISTER_FILTER_CLASS_INSTANCE(classEntry, className) \
    classEntry::Registrar classEntry::_registrar(className, Filter::getFactory());


#endif
