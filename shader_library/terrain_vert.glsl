#version 450
#pragma shader_stage(vertex)

layout(binding = 0) uniform MAT {
    mat4 viewProjection;
    mat4 model;
} matrix_data;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main() {
    gl_Position = (matrix_data.viewProjection * matrix_data.model) * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	fragNormal = inNormal;
}