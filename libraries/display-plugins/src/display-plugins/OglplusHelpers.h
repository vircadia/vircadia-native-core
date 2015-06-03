//
//  Created by Bradley Austin Davis on 2015/05/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <GLMHelpers.h>

#pragma warning(disable : 4068)

#define OGLPLUS_USE_GLEW 1
#define OGLPLUS_USE_GLCOREARB_H 0
#define OGLPLUS_USE_BOOST_CONFIG 1
#define OGLPLUS_NO_SITE_CONFIG 1
#define OGLPLUS_LOW_PROFILE 1
#include <GL/glew.h>
#include <oglplus/gl.hpp>

#include <oglplus/all.hpp>
#include <oglplus/interop/glm.hpp>
#include <oglplus/bound/texture.hpp>
#include <oglplus/bound/framebuffer.hpp>
#include <oglplus/bound/renderbuffer.hpp>
#include <oglplus/shapes/wrapper.hpp>
#include <oglplus/shapes/plane.hpp>

#include <NumericalConstants.h>

typedef std::shared_ptr<oglplus::Framebuffer> FramebufferPtr;
typedef std::shared_ptr<oglplus::shapes::ShapeWrapper> ShapeWrapperPtr;
typedef std::shared_ptr<oglplus::Buffer> BufferPtr;
typedef std::shared_ptr<oglplus::VertexArray> VertexArrayPtr;
typedef std::shared_ptr<oglplus::Program> ProgramPtr;
typedef oglplus::Uniform<mat4> Mat4Uniform;

ProgramPtr loadDefaultShader();
void compileProgram(ProgramPtr & result, const std::string& vs, const std::string& fs);
ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect = 1.0f);
ShapeWrapperPtr loadSphereSection(ProgramPtr program, float fov = PI / 3.0f * 2.0f, float aspect = 16.0f / 9.0f, int slices = 80, int stacks = 80);
    
