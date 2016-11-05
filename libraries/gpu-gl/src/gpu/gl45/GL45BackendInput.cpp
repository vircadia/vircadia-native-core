//
//  GL45BackendInput.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"
#include "../gl/GLShared.h"

using namespace gpu;
using namespace gpu::gl45;

void GL45Backend::resetInputStage() {
    Parent::resetInputStage();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (uint32_t i = 0; i < _input._attributeActivation.size(); i++) {
        glDisableVertexAttribArray(i);
    }
    for (uint32_t i = 0; i < _input._attribBindingBuffers.size(); i++) {
        glBindVertexBuffer(i, 0, 0, 0);
    }
}

void GL45Backend::updateInput() {
    if (_input._invalidFormat) {
        InputStageState::ActivationCache newActivation;

        // Assign the vertex format required
        if (_input._format) {
            _input._attribBindingBuffers.reset();

            const Stream::Format::AttributeMap& attributes = _input._format->getAttributes();
            auto& inputChannels = _input._format->getChannels();
            for (auto& channelIt : inputChannels) {
                auto bufferChannelNum = (channelIt).first;
                const Stream::Format::ChannelMap::value_type::second_type& channel = (channelIt).second;
                _input._attribBindingBuffers.set(bufferChannelNum);

                GLuint frequency = 0;
                for (unsigned int i = 0; i < channel._slots.size(); i++) {
                    const Stream::Attribute& attrib = attributes.at(channel._slots[i]);

                    GLuint slot = attrib._slot;
                    GLuint count = attrib._element.getLocationScalarCount();
                    uint8_t locationCount = attrib._element.getLocationCount();
                    GLenum type = gl::ELEMENT_TYPE_TO_GL[attrib._element.getType()];

                    GLuint offset = (GLuint)attrib._offset;;
                    GLboolean isNormalized = attrib._element.isNormalized();

                    GLenum perLocationSize = attrib._element.getLocationSize();
                    for (GLuint locNum = 0; locNum < locationCount; ++locNum) {
                        GLuint attriNum = (GLuint)(slot + locNum);
                        newActivation.set(attriNum);
                        if (!_input._attributeActivation[attriNum]) {
                            _input._attributeActivation.set(attriNum);
                            glEnableVertexAttribArray(attriNum);
                        }
                        glVertexAttribFormat(attriNum, count, type, isNormalized, offset + locNum * perLocationSize);
                        // TODO: Support properly the IAttrib version
                        glVertexAttribBinding(attriNum, attrib._channel);
                    }

                    if (i == 0) {
                        frequency = attrib._frequency;
                    } else {
                        assert(frequency == attrib._frequency);
                    }

                    (void)CHECK_GL_ERROR();
                }
                glVertexBindingDivisor(bufferChannelNum, frequency);
            }


            // Manage Activation what was and what is expected now
            // This should only disable VertexAttribs since the one in use have been disabled above
            for (GLuint i = 0; i < (GLuint)newActivation.size(); i++) {
                bool newState = newActivation[i];
                if (newState != _input._attributeActivation[i]) {
                    if (newState) {
                        glEnableVertexAttribArray(i);
                    } else {
                        glDisableVertexAttribArray(i);
                    }
                    _input._attributeActivation.flip(i);
                }
            }
            (void)CHECK_GL_ERROR();
        }

        _input._invalidFormat = false;
        _stats._ISNumFormatChanges++;
    }

    if (_input._invalidBuffers.any()) {
        auto vbo = _input._bufferVBOs.data();
        auto offset = _input._bufferOffsets.data();
        auto stride = _input._bufferStrides.data();

        for (GLuint buffer = 0; buffer < _input._buffers.size(); buffer++, vbo++, offset++, stride++) {
            if (_input._invalidBuffers.test(buffer)) {
                glBindVertexBuffer(buffer, (*vbo), (*offset), (GLsizei)(*stride));
            }
        }

        _input._invalidBuffers.reset();
        (void)CHECK_GL_ERROR();
    }
}
