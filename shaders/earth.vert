#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    vWorldPos = vec3(uModel * vec4(inPos, 1.0));
    vNormal   = mat3(transpose(inverse(uModel))) * inNormal;

    gl_Position = uMVP * vec4(inPos, 1.0);
}
