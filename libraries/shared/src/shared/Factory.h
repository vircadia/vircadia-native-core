//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Shared_Factory_h
#define hifi_Shared_Factory_h

#include <functional>
#include <map>
#include <memory>

namespace hifi {

    template <typename T, typename Key>
    class SimpleFactory {
    public:
        using Pointer = std::shared_ptr<T>;
        using Builder = std::function<Pointer()>;
        using BuilderMap = std::map<Key, Builder>;

        void registerBuilder(const Key name, Builder builder) {
            // FIXME don't allow name collisions
            _builders[name] = builder;
        }

        Pointer create(const Key name) const {
            const auto& entryIt = _builders.find(name);
            if (entryIt != _builders.end()) {
                return (*entryIt).second();
            }
            return Pointer();
        }

        template <typename Impl>
        class Registrar {
        public:
            Registrar(const Key name, SimpleFactory& factory) {
                factory.registerBuilder(name, [] { return std::make_shared<Impl>(); });
            }
        };
    protected:
        BuilderMap _builders;
    };
}

#endif
