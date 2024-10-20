#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    /* vec4 finalColor = texture(texSampler, fragTexCoord).rgba;

    if(finalColor.a < 0.2)
    {
        discard;
    } */

    outColor = vec4(fragColor, 1);
}