//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Endpoint_h
#define hifi_Controllers_Endpoint_h

#include <list>
#include <memory>
#include <functional>

#include <QtCore/QObject>

#include "../Input.h"
#include "../Pose.h"

class QScriptValue;

namespace controller {
    /*
    * Encapsulates a particular input / output,
    * i.e. Hydra.Button0, Standard.X, Action.Yaw
    */
    class Endpoint : public QObject {
        Q_OBJECT;
    public:
        using Pointer = std::shared_ptr<Endpoint>;
        using List = std::list<Pointer>;
        using Pair = std::pair<Pointer, Pointer>;
        using ReadLambda = std::function<float()>;
        using WriteLambda = std::function<void(float)>;

        Endpoint(const Input& input) : _input(input) {}
        virtual float value() { return peek(); }
        virtual float peek() const = 0;
        virtual void apply(float value, const Pointer& source) = 0;
        virtual Pose peekPose() const { return Pose(); };
        virtual Pose pose() { return peekPose(); }
        virtual void apply(const Pose& value, const Pointer& source) {}
        virtual bool isPose() const { return _input.isPose(); }
        virtual bool writeable() const { return true; }
        virtual bool readable() const { return true; }
        virtual void reset() { }

        const Input& getInput() { return _input;  }

    protected:
        Input _input;
    };

    class LambdaEndpoint : public Endpoint {
    public:
        using Endpoint::apply;
        LambdaEndpoint(ReadLambda readLambda, WriteLambda writeLambda = [](float) {})
            : Endpoint(Input::INVALID_INPUT), _readLambda(readLambda), _writeLambda(writeLambda) { }

        virtual float peek() const override { return _readLambda(); }
        virtual void apply(float value, const Pointer& source) override { _writeLambda(value); }

    private:
        ReadLambda _readLambda;
        WriteLambda _writeLambda;
    };

    extern Endpoint::WriteLambda DEFAULT_WRITE_LAMBDA;

    class LambdaRefEndpoint : public Endpoint {
    public:
        using Endpoint::apply;
        LambdaRefEndpoint(const ReadLambda& readLambda, const WriteLambda& writeLambda = DEFAULT_WRITE_LAMBDA)
            : Endpoint(Input::INVALID_INPUT), _readLambda(readLambda), _writeLambda(writeLambda) {
        }

        virtual float peek() const override { return _readLambda(); }
        virtual void apply(float value, const Pointer& source) override { _writeLambda(value); }

    private:
        const ReadLambda& _readLambda;
        const WriteLambda& _writeLambda;
    };


    class VirtualEndpoint : public Endpoint {
    public:
        VirtualEndpoint(const Input& id = Input::INVALID_INPUT)
            : Endpoint(id) {
        }

        virtual float peek() const override { return _currentValue; }
        virtual void apply(float value, const Pointer& source) override { _currentValue = value; }

        virtual Pose peekPose() const override { return _currentPose; }
        virtual void apply(const Pose& value, const Pointer& source) override {
            _currentPose = value;
        }
    protected:
        float _currentValue { 0.0f };
        Pose _currentPose {};
    };

}

#endif
