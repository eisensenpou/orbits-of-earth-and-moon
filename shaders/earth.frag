#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 lightDir;   // normalized
uniform vec3 baseColor;

void main() {
    
    float diff = max(dot(N, L), 0.0);
    
    float ambient = 0.5;   // MUCH brighter
    
    FragColor = vec4(uColor * (ambient + diff), 1.0);

}