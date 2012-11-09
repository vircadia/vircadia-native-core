//
//  field.cpp
//  interface
//
//  Created by Philip Rosedale on 8/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "field.h"
#define FIELD_SCALE 0.00050

//  A vector-valued field over an array of elements arranged as a 3D lattice 

struct {
    glm::vec3 val;
} field[FIELD_ELEMENTS];


int field_value(float *value, float *pos)
// sets the vector value (3 floats) to field value at location pos in space.
// returns zero if the location is outside world bounds
{
    int index = (int)(pos[0]/WORLD_SIZE*10.0) + 
    (int)(pos[1]/WORLD_SIZE*10.0)*10 + 
    (int)(pos[2]/WORLD_SIZE*10.0)*100;
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        value[0] = field[index].val.x;
        value[1] = field[index].val.y;
        value[2] = field[index].val.z;
        return 1;
    }
    else return 0;
}


void field_init()
//  Initializes the field to some random values
{
    int i;
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        field[i].val.x = (randFloat() - 0.5)*FIELD_SCALE;
        field[i].val.y = (randFloat() - 0.5)*FIELD_SCALE;
        field[i].val.z = (randFloat() - 0.5)*FIELD_SCALE;
    }       
}

void field_add(float* add, float *pos)
//  At location loc, add vector add to the field values 
{
    int index = (int)(pos[0]/WORLD_SIZE*10.0) + 
    (int)(pos[1]/WORLD_SIZE*10.0)*10 + 
    (int)(pos[2]/WORLD_SIZE*10.0)*100;
    if ((index >= 0) && (index < FIELD_ELEMENTS))
    {
        field[index].val.x += add[0];
        field[index].val.y += add[1];
        field[index].val.z += add[2];
    }
}

void field_avg_neighbors(int index, glm::vec3 * result) {
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

void field_simulate(float dt) {
    glm::vec3 neighbors, add;
    float size;
    for (int i = 0; i < FIELD_ELEMENTS; i++)
    {
        if (0) { //(randFloat() > 0.01) {
            field_avg_neighbors(i, &neighbors);
            size = powf(field[i].val.x*field[i].val.x +
                        field[i].val.y*field[i].val.y + 
                        field[i].val.z*field[i].val.z, 0.5);
            
            neighbors *= 0.0001;
            // not currently in use
            // glm::vec3 test = glm::normalize(glm::vec3(0,0,0));
            
            field[i].val = glm::normalize(field[i].val);
            field[i].val *= size * 0.99;
            add = glm::normalize(neighbors); 
            add *= size * 0.01;
            field[i].val += add;
        }
        else {
            field[i].val *= 0.999;
            //field[i].val.x += (randFloat() - 0.5)*0.01*FIELD_SCALE;
            //field[i].val.y += (randFloat() - 0.5)*0.01*FIELD_SCALE;
            //field[i].val.z += (randFloat() - 0.5)*0.01*FIELD_SCALE;
        }
        
    }      
}

void field_render()
//  Render the field lines
{
    int i;
    float fx, fy, fz;
    float scale_view = 1000.0;
    
    glDisable(GL_LIGHTING);
    glColor3f(0, 1, 0);
    glBegin(GL_LINES);
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        fx = (int)(i % 10);
        fy = (int)(i%100 / 10);
        fz = (int)(i / 100);
        
        glVertex3f(fx, fy, fz);
        glVertex3f(fx + field[i].val.x*scale_view, 
                   fy + field[i].val.y*scale_view, 
                   fz + field[i].val.z*scale_view);
        
    }
    glEnd();
    
    glColor3f(0, 1, 0);
    glPointSize(4.0);
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
    
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        fx = (int)(i % 10);
        fy = (int)(i%100 / 10);
        fz = (int)(i / 100);
        
        glVertex3f(fx, fy, fz);
    }
    glEnd();
}


