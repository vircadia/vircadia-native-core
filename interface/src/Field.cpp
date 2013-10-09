//
//  Field.cpp
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include "Field.h"

#define FIELD_SCALE 0.00050
#define COLOR_DRIFT_RATE 0.001f // per-frame drift of particle color towards field element color
#define COLOR_MIN 0.2f // minimum R/G/B value at 0,0,0 - also needs setting in cloud.cpp


//  A vector-valued field over an array of elements arranged as a 3D lattice 

int Field::value(float *value, float *pos)
// sets the vector value (3 floats) to field value at location pos in space.
// returns zero if the location is outside world bounds
{
    int index = (int)(pos[0] / _worldSize * 10.0) +
    (int)(pos[1] / _worldSize * 10.0) * 10 + 
    (int)(pos[2] / _worldSize * 10.0) * 100;
    
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        value[0] = field[index].val.x;
        value[1] = field[index].val.y;
        value[2] = field[index].val.z;
        return 1;
    }
    else return 0;
}

Field::Field(float worldSize)
//  Initializes the field to some random values
{
    _worldSize = worldSize;
    int i;
    float fx, fy, fz;
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        const float FIELD_INITIAL_MAG = 0.3f;
        field[i].val = randVector() * FIELD_INITIAL_MAG * _worldSize;
        field[i].scalar = 0; 
        //  Record center point for this field cell
        fx = static_cast<float>(i % 10);
        fy = static_cast<float>(i % 100 / 10);
        fz = static_cast<float>(i / 100);
        field[i].center.x = (fx + 0.5f);
        field[i].center.y = (fy + 0.5f);
        field[i].center.z = (fz + 0.5f);
        field[i].center *= _worldSize / 10.f;
        
        // and set up the RGB values for each field element.
        float color_mult = 1 - COLOR_MIN;
        fieldcolors[i].rgb = glm::vec3(((i%10)*(color_mult/10.0f)) + COLOR_MIN,
                                       ((i%100)*(color_mult/100.0f)) + COLOR_MIN,
                                       (i*(color_mult/1000.0f)) + COLOR_MIN);
    }
}

void Field::add(float* add, float *pos)
//  At location loc, add vector add to the field values 
{
    int index = (int)(pos[0] / _worldSize * 10.0) + 
    (int)(pos[1] / _worldSize * 10.0)*10 +
    (int)(pos[2] / _worldSize * 10.0)*100;
    
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        field[index].val.x += add[0];
        field[index].val.y += add[1];
        field[index].val.z += add[2];
    }
}

void Field::interact(float dt, glm::vec3 * pos, glm::vec3 * vel, glm::vec3 * color, float coupling) {
     
    int index = (int)(pos->x/ _worldSize * 10.0) +
    (int)(pos->y/_worldSize*10.0)*10 + 
    (int)(pos->z/_worldSize*10.0)*100;
    if ((index >= 0) && (index < FIELD_ELEMENTS)) {
        //  
        //  Vector Coupling with particle velocity
        //
        *vel += field[index].val*dt;            //  Particle influenced by field
        
        glm::vec3 temp = *vel*dt;               //  Field influenced by particle
        temp *= coupling;
        field[index].val += temp;
        // 
        //  Scalar coupling:  Damp particle as function of local density  
        // 
        
        // add a fraction of the field color to the particle color
        //*color = (*color * (1 - COLOR_DRIFT_RATE)) + (fieldcolors[index].rgb * COLOR_DRIFT_RATE);
    }
}

void Field::avg_neighbors(int index, glm::vec3 * result) {
    //  Given index to field element i, return neighbor field values 
    glm::vec3 neighbors(0,0,0);
    
    int x,y,z;
    x = (int)(index % 10);
    y = (int)(index%100 / 10);
    z = (int)(index / 100);

    neighbors += field[(x+1)%10 + y*10 + z*100].val;
    neighbors += field[(x-1)%10 + y*10 + z*100].val;
    
    neighbors += field[x + ((y+1)%10)*10 + z*100].val;    
    neighbors += field[x + ((y-1)%10)*10 + z*100].val;  
    
    neighbors += field[x + y*10 + ((z+1)%10)*100].val;    
    neighbors += field[x%10 + y*10 + ((z-1)%10)*100].val;  
    
    neighbors /= 6;
    result->x = neighbors.x;
    result->y = neighbors.y;
    result->z = neighbors.z;
    
}

void Field::simulate(float dt) {
    glm::vec3 neighbors, add, diff;

    for (int i = 0; i < FIELD_ELEMENTS; i++)
    {
        const float CONSTANT_DAMPING = 0.5;
        const float CONSTANT_SCALAR_DAMPING = 2.5;
        field[i].val *= (1.f - CONSTANT_DAMPING*dt);
        field[i].scalar *= (1.f - CONSTANT_SCALAR_DAMPING*dt);
    }
}

void Field::render()
//  Render the field lines
{
    int i;
    float scale_view = 0.05f * _worldSize;
    
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {        
        glColor3f(0, 1, 0);
        glVertex3fv(&field[i].center.x);
        glVertex3f(field[i].center.x + field[i].val.x * scale_view,
                   field[i].center.y + field[i].val.y * scale_view,
                   field[i].center.z + field[i].val.z * scale_view);
    }
    glEnd();
    
    glColor3f(0, 1, 0);
    glPointSize(4.0);
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        glVertex3fv(&field[i].center.x);
    }
    glEnd();
}



