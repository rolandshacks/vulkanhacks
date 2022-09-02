//
// Fragment Shader
//

#version 450

const float PI = 3.1415926538;

layout(std140, set=0, binding=0) uniform shader_params {
    float resolution_x;
    float resolution_y;
    float x_min;
    float x_max;
    float y_min;
    float y_max;
    float time;
    float time_delta;
    int frame;
} params;

layout (binding = 1) uniform sampler2D iTexture;
layout (binding = 2) uniform sampler2D iTexture2;

layout (location = 0) in vertex_data {
    vec4 position;
    vec4 color;
    vec2 textureCoord;
    flat uint textureMask;
    flat uint flags;
} inputs;

layout (location = 0) out vec4 oColor;

vec4 calculateHighlight(in vec2 fragCoord) {
    float distance_x = min(1.0, abs(fragCoord.x));
    float distance_y = min(1.0, abs(fragCoord.y));

    float distance = min(1.0, abs(fragCoord.x * fragCoord.y));
    float intensity = pow(cos(distance * 3.1415 / 2.0 ), 100.0);

    float d = intensity;

    return vec4(d, d, d, 0.0);
}

void main() {
    vec2 fragCoord = inputs.position.xy;
    vec4 fragColor = inputs.color;
    fragColor += calculateHighlight(fragCoord);

    if (0x0 != (inputs.textureMask & 0x1)) {
        fragColor *= texture(iTexture, inputs.textureCoord);
    }

    if (0x0 != (inputs.textureMask & 0x2)) {
        fragColor *= texture(iTexture2, inputs.textureCoord);
    }

    oColor = fragColor;
}
