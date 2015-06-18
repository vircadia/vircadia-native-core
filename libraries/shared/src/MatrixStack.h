//
//  Created by Bradley Austin Davis
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stack>

class MatrixStack : public std::stack<glm::mat4> {

public:

  MatrixStack() {
    push(glm::mat4());
  }

  explicit MatrixStack(const MatrixStack& other) : std::stack<glm::mat4>() {
    *((std::stack<glm::mat4>*)this) = *((std::stack<glm::mat4>*)&other);
  }

  operator const glm::mat4 & () const {
    return top();
  }

  MatrixStack & pop() {
    std::stack<glm::mat4>::pop();
    assert(!empty());
    return *this;
  }

  MatrixStack & push() {
    emplace(top());
    return *this;
  }

  MatrixStack & identity() {
    top() = glm::mat4();
    return *this;
  }

  MatrixStack & push(const glm::mat4 & mat) {
    std::stack<glm::mat4>::push(mat);
    return *this;
  }

  MatrixStack & rotate(const glm::mat3 & rotation) {
    return postMultiply(glm::mat4(rotation));
  }

  MatrixStack & rotate(const glm::quat & rotation) {
    return postMultiply(glm::mat4_cast(rotation));
  }

  MatrixStack & rotate(float theta, const glm::vec3 & axis) {
    return postMultiply(glm::rotate(glm::mat4(), theta, axis));
  }

  MatrixStack & translate(float translation) {
    return translate(glm::vec3(translation, 0, 0));
  }

  MatrixStack & translate(const glm::vec2 & translation) {
    return translate(glm::vec3(translation, 0));
  }

  MatrixStack & translate(const glm::vec3 & translation) {
    return postMultiply(glm::translate(glm::mat4(), translation));
  }

  MatrixStack & preTranslate(const glm::vec3 & translation) {
    return preMultiply(glm::translate(glm::mat4(), translation));
  }


  MatrixStack & scale(float factor) {
    return scale(glm::vec3(factor));
  }

  MatrixStack & scale(const glm::vec3 & scale) {
    return postMultiply(glm::scale(glm::mat4(), scale));
  }

  MatrixStack & transform(const glm::mat4 & xfm) {
    return postMultiply(xfm);
  }

  MatrixStack & preMultiply(const glm::mat4 & xfm) {
    top() = xfm * top();
    return *this;
  }

  MatrixStack & postMultiply(const glm::mat4 & xfm) {
    top() *= xfm;
    return *this;
  }

  // Remove the rotation component of a matrix.  useful for billboarding
  MatrixStack & unrotate() {
    glm::quat inverse = glm::inverse(glm::quat_cast(top()));
    top() = top() * glm::mat4_cast(inverse);
    return *this;
  }

  // Remove the translation component of a matrix.  useful for skyboxing
  MatrixStack & untranslate() {
    top()[3] = glm::vec4(0, 0, 0, 1);
    return *this;
  }

  template <typename Function>
  void withPush(Function f) {
    #ifndef NDEBUG
    size_t startingDepth = size();
    #endif
    push();
    f();
    pop();
    #ifndef NDEBUG
    assert(startingDepth == size());
    #endif
  }

  template <typename Function>
  void withIdentity(Function f) {
    withPush([&] {
      identity();
      f();
    });
  }

  static MatrixStack & projection() {
    static MatrixStack projection;
    return projection;
  }

  static MatrixStack & modelview() {
    static MatrixStack modelview;
    return modelview;
  }

  template <typename Function>
  static void withPushAll(Function f) {
    withPush(projection(), modelview(), f);
  }

  template <typename Function>
  static void withIdentityAll(Function f) {
    withPush(projection(), modelview(), [=] {
      projection().identity();
      modelview().identity();
      f();
    });
  }

  template <typename Function>
  static void withPush(MatrixStack& stack, Function f) {
    stack.withPush(f);
  }

  template <typename Function>
  static void withPush(MatrixStack& stack1, MatrixStack& stack2, Function f) {
    stack1.withPush([&]{
      stack2.withPush(f);
    });
  }
};

