//
//  ModelPackager.h
//
//
//  Created by Clement on 3/9/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelPackager_h
#define hifi_ModelPackager_h

class ModelPackager {
public:
    static void package();
    
private:
    bool selectModel();
    void editProperties();
    void zipModel();
};




#endif // hifi_ModelPackager_h