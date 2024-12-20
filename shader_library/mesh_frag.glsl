#version 450
#pragma shader_stage(fragment)

layout(set = 0, binding = 1) uniform sampler2D texSampler[16];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[0], fragTexCoord).rgba;
}