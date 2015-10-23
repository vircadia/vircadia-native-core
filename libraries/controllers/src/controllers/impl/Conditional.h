//
//  Created by Bradley Austin Davis 2015/10/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Conditional_h
#define hifi_Controllers_Conditional_h

#include <list>
#include <memory>

#include <QtCore/QString>

#include <shared/Factory.h>

class QJsonValue;

namespace controller {
    /*
    * encapsulates a source, destination and filters to apply
    */
    class Conditional {
    public:
        using Pointer = std::shared_ptr<Conditional>;
        using List = std::list<Pointer>;
        using Factory = hifi::SimpleFactory<Conditional, QString>;
        using Lambda = std::function<bool()>;

        virtual bool satisfied() = 0;
        virtual bool parseParameters(const QJsonValue& parameters) { return true; }

        static Pointer parse(const QJsonValue& json);
        static void registerBuilder(const QString& name, Factory::Builder builder);
        static Factory& getFactory() { return _factory; }
    protected:
        static Factory _factory;
    };

}

#define REGISTER_CONDITIONAL_CLASS(classEntry) \
    private: \
    using Registrar = Conditional::Factory::Registrar<classEntry>; \
    static Registrar _registrar;

#define REGISTER_CONDITIONAL_CLASS_INSTANCE(classEntry, className) \
    classEntry::Registrar classEntry::_registrar(className, Conditional::getFactory());


#endif
