#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler;

void main() {
    //outColor = vec4(fragTexCoord, 0.0, 1.0);
    vec2 flippedTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    outColor = texture(texSampler, flippedTexCoord);
}

