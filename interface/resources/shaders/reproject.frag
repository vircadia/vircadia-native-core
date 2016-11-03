#version 450 core

uniform sampler2D sampler;
layout (location = 0) uniform mat3 reprojection = mat3(1);
layout (location = 4) uniform mat4 inverseProjections[2];
layout (location = 12) uniform mat4 projections[2];

in vec2 vTexCoord;
in vec3 vPosition;

out vec4 FragColor;

void main() {
    vec2 uv = vTexCoord;

    mat4 eyeInverseProjection;
    mat4 eyeProjection;
    
    float xoffset = 1.0;
    vec2 uvmin = vec2(0.0);
    vec2 uvmax = vec2(1.0);
    // determine the correct projection and inverse projection to use.
    if (vTexCoord.x < 0.5) {
        uvmax.x = 0.5;
        eyeInverseProjection = inverseProjections[0];
        eyeProjection = projections[0];
    } else {
        xoffset = -1.0;
        uvmin.x = 0.5;
        uvmax.x = 1.0;
        eyeInverseProjection = inverseProjections[1];
        eyeProjection = projections[1];
    }

    // Account for stereo in calculating the per-eye NDC coordinates
    vec4 ndcSpace = vec4(vPosition, 1.0);
    ndcSpace.x *= 2.0;
    ndcSpace.x += xoffset;
    
    // Convert from NDC to eyespace
    vec4 eyeSpace = eyeInverseProjection * ndcSpace;
    eyeSpace /= eyeSpace.w;

    // Convert to a noramlized ray 
    vec3 ray = eyeSpace.xyz;
    ray = normalize(ray);

    // Adjust the ray by the rotation
    ray = reprojection * ray;

    // Project back on to the texture plane
    ray /= ray.z;
    ray *= eyeSpace.z;

    // Update the eyespace vector
    eyeSpace.xyz = ray;

    // Reproject back into NDC
    ndcSpace = eyeProjection * eyeSpace;
    ndcSpace /= ndcSpace.w;
    ndcSpace.x -= xoffset;
    ndcSpace.x /= 2.0;
    
    // Calculate the new UV coordinates
    uv = (ndcSpace.xy / 2.0) + 0.5;
    if (any(greaterThan(uv, uvmax)) || any(lessThan(uv, uvmin))) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = texture(sampler, uv);
    }
}
