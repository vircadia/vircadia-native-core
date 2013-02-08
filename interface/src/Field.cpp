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

#define USE_SCALAR 0 

//  A vector-valued field over an array of elements arranged as a 3D lattice 

int Field::value(float *value, float *pos)
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

Field::Field()
//  Initializes the field to some random values
{
    int i;
    float fx, fy, fz;
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        field[i].val.x = (randFloat() - 0.5)*FIELD_SCALE;
        field[i].val.y = (randFloat() - 0.5)*FIELD_SCALE;
        field[i].val.z = (randFloat() - 0.5)*FIELD_SCALE;
        field[i].scalar = 0; 
        //  Record center point for this field cell
        fx = (int)(i % 10);
        fy = (int)(i%100 / 10);
        fz = (int)(i / 100);
        field[i].center.x = fx + 0.5;
        field[i].center.y = fy + 0.5;
        field[i].center.z = fz + 0.5;
        
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

void Field::interact(float dt, glm::vec3 * pos, glm::vec3 * vel, glm::vec3 * color, float coupling) {
     
    int index = (int)(pos->x/WORLD_SIZE*10.0) + 
    (int)(pos->y/WORLD_SIZE*10.0)*10 + 
    (int)(pos->z/WORLD_SIZE*10.0)*100;
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
        
        if (USE_SCALAR) {
            //*vel *= (1.f + field[index].scalar*0.01*dt); 
            const float SCALAR_PARTICLE_ADD = 1.0;
            field[index].scalar += SCALAR_PARTICLE_ADD*dt;
        }
        
        
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
    float size, distance;
    int i, j;
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        if (0) { //(randFloat() > 0.01) {
            avg_neighbors(i, &neighbors);
            size = powf(field[i].val.x*field[i].val.x +
                        field[i].val.y*field[i].val.y + 
                        field[i].val.z*field[i].val.z, 0.5);
            
            neighbors *= 0.0001;
            field[i].val = glm::normalize(field[i].val);
            field[i].val *= size * 0.99;
            add = glm::normalize(neighbors); 
            add *= size * 0.01;
            field[i].val += add;
        }
        else {
            const float CONSTANT_DAMPING = 0.5;
            const float CONSTANT_SCALAR_DAMPING = 2.5;
            field[i].val *= (1.f - CONSTANT_DAMPING*dt);
            field[i].scalar *= (1.f - CONSTANT_SCALAR_DAMPING*dt);
        }
        
        if (USE_SCALAR) {
            // 
            // Compute a field value from sum of all other field values (electrostatics, etc)
            //
            field[i].fld.x = field[i].fld.y = field[i].fld.z = 0;
            for (j = 0; j < FIELD_ELEMENTS; j++)
            {
                if (i != j) {
                    //  Compute vector field from scalar densities
                    diff = field[j].center - field[i].center;
                    distance = glm::length(diff);
                    diff = glm::normalize(diff);
                    field[i].fld += diff*field[j].scalar*(1/distance);
                }
            }
        }
    }      
}

void Field::render()
//  Render the field lines
{
    int i;
    float fx, fy, fz;
    float scale_view = 0.1;
    
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    for (i = 0; i < FIELD_ELEMENTS; i++)
    {
        fx = field[i].center.x;
        fy = field[i].center.y;
        fz = field[i].center.z;
        
        glColor3f(0, 1, 0);
        glVertex3f(fx, fy, fz);
        glVertex3f(fx + field[i].val.x*scale_view, 
                   fy + field[i].val.y*scale_view, 
                   fz + field[i].val.z*scale_view);
        if (USE_SCALAR) {
            glColor3f(1, 0, 0);
            glVertex3f(fx, fy, fz);        
            glVertex3f(fx, fy+field[i].scalar*0.01, fz);
            glColor3f(1, 1, 0);
            glVertex3f(fx, fy, fz);
            glVertex3f(fx + field[i].fld.x*0.0001, 
                       fy + field[i].fld.y*0.0001, 
                       fz + field[i].fld.z*0.0001);
        }

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



