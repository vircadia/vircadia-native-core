
float getProceduralColors(inout vec3 diffuse, inout vec3 specular, inout float shininess) {
    
    specular = _modelNormal.rgb;
    if (any(lessThan(specular, vec3(0.0)))) {
        specular = vec3(1.0) + specular;
    }
    diffuse = vec3(1.0, 1.0, 1.0);
    return 1.0;
}