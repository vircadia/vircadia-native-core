#version 120
#extension GL_ARB_geometry_shader4 : enable

// use GL_POINTS
// have point be the corner of voxel
// have a second dataset (? similar to how voxel avatars pass in bones??)
//     which is the voxel size?
//
// In vertex shader DON'T transform.. therefor passing the world coordinate xyz to geometric shader
// In geometric shader calculate xyz for triangles the same way we currently do triangles outside of OpenGL
// do transform on these triangles
//            gl_Position = gl_ModelViewProjectionMatrix * cube_coord;
//
// output GL_TRIANGLE_STRIP
//
// NOTE: updateNodeInArrays() does the covert from voxel corner to 12 triangles or 36 points or 36*3 floats
//       but since it operates on the array of floats, it is kinda weird and hard to follow. The %3 is for the
//       xyz.. and identityVertices[j] term in the math is actually a string of floats but they should be thought
//       of as triplets of x,y,z
//
// do we need to add the light to these colors?? 
//

//GEOMETRY SHADER
///////////////////////
void main()
{
    //increment variable
    int i;
    vec4 vertex;
    vec4 color,red,green,blue;

    green = vec4(0,1.0,0,1.0);
    red = vec4(1.0,0,0,1.0);
    blue = vec4(0,0,1.0,1.0);
    
    /////////////////////////////////////////////////////////////
    //This example has two parts
    //   step a) draw the primitive pushed down the pipeline
    //            there are gl_VerticesIn # of vertices
    //            put the vertex value into gl_Position
    //            use EmitVertex => 'create' a new vertex
    //           use EndPrimitive to signal that you are done creating a primitive!
    //   step b) create a new piece of geometry
    //           I just do the same loop, but I negate the vertex.z
    //   result => the primitive is now mirrored.
    //Pass-thru!
    for(i = 0; i < gl_VerticesIn; i++) {
        color = gl_FrontColorIn[i];                
        gl_FrontColor = color; // 
        gl_Position = gl_ModelViewProjectionMatrix * gl_PositionIn[i];
        EmitVertex();
    }
    EndPrimitive();

    for(i = 0; i < gl_VerticesIn; i++) {
        gl_FrontColor = red; // 
        vertex = gl_PositionIn[i];
        vertex.y += 0.05f;
        gl_Position = gl_ModelViewProjectionMatrix * vertex;
        EmitVertex();
    }
    EndPrimitive();

    for(i = 0; i < gl_VerticesIn; i++) {
        gl_FrontColor = green; // 
        vertex = gl_PositionIn[i];
        vertex.x += 0.05f;
        gl_Position = gl_ModelViewProjectionMatrix * vertex;
        EmitVertex();
    }
    EndPrimitive();

    for(i = 0; i < gl_VerticesIn; i++) {
        gl_FrontColor = blue; // 
        vertex = gl_PositionIn[i];
        vertex.z += 0.05f;
        gl_Position = gl_ModelViewProjectionMatrix * vertex;
        EmitVertex();
    }
    EndPrimitive();

/**

    for(i = 0; i < gl_VerticesIn; i++) {
        green = vec4(0,1.0,0,1.0);
        red = vec4(1.0,0,0,1.0);
        //gl_FrontColor = gl_FrontColorIn[i];
        gl_FrontColor = red;

        // v -> v+x -> v+x+y
        vertex = gl_PositionIn[i];
        gl_Position = vertex;
        EmitVertex();
        vertex.x += 0.1f;
        gl_Position = vertex;
        EmitVertex();
        vertex.y += 0.1f;
        gl_Position = vertex;
        EmitVertex();
        EndPrimitive();

        // v+x+y -> v+y -> v
        vertex = gl_PositionIn[i];
        vertex.x += 0.1f;
        vertex.y += 0.1f;
        gl_Position = vertex;
        EmitVertex();
        vertex.x -= 0.1f;
        gl_Position = vertex;
        EmitVertex();
        vertex.y -= 0.1f;
        gl_Position = vertex;
        EmitVertex();
        EndPrimitive();
        // v+z -> v+z+x -> v+z+x+y
        gl_FrontColor = green;
        vertex = gl_PositionIn[i];
        vertex.z -= 0.1f;
        gl_Position = vertex;
        EmitVertex();

        vertex.x += 0.1f;
        gl_Position = vertex;
        EmitVertex();
        vertex.y += 0.1f;
        gl_Position = vertex;
        EmitVertex();
        EndPrimitive();

        // v+z+x+y -> v+z+y -> v+z
        vertex = gl_PositionIn[i];
        vertex.z -= 0.1f;
        vertex.x += 0.1f;
        vertex.y += 0.1f;
        gl_Position = vertex;
        EmitVertex();
        vertex.x -= 0.1f;
        gl_Position = vertex;
        EmitVertex();
        vertex.y -= 0.1f;
        gl_Position = vertex;
        EmitVertex();
        EndPrimitive();

    }
**/


}