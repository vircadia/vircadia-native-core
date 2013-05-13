//
//  PointerStack.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 4/25/2013
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__PointerStack__
#define __hifi__PointerStack__

#include <cstring> // for NULL

class PointerStack {

public:
    PointerStack() : 
        _elements(NULL),
        _elementsInUse(0),
        _sizeOfElementsArray(0) {};
        
    ~PointerStack();
    
    void push(void* element) {
        if (_sizeOfElementsArray < _elementsInUse + 1) {
            return growAndPush(element);
        }
        _elements[_elementsInUse] = element;
        _elementsInUse++;
    };
 
    void* pop() {
        if (_elementsInUse) {
            // get the last element
            void* element = _elements[_elementsInUse - 1];
            // reduce the count
            _elementsInUse--;
            return element;
        }
        return NULL;
    };


    void* top() const { return (_elementsInUse) ? _elements[_elementsInUse - 1] : NULL; }    
    bool isEmpty() const { return (_elementsInUse == 0); };
    bool empty() const { return (_elementsInUse == 0); };
    int count() const { return _elementsInUse; };
    int size() const { return _elementsInUse; };

private:
    void growAndPush(void* element);
    void deleteAll();
    void**      _elements;
    int         _elementsInUse;
    int         _sizeOfElementsArray;
};

#endif /* defined(__hifi__PointerStack__) */
