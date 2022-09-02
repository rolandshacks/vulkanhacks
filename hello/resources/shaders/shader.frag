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

layout (location = 0) in vertex_data {
    vec4 position;
    vec4 color;
    vec2 textureCoord;
    flat uint textureMask;
    flat uint flags;
} inputs;

layout (location = 0) out vec4 oColor;


vec4 calculateLight(in vec2 fragCoord, in vec4 baseColor, in float scale, in float offset) {

    float lightPos = fract(params.time / 10.0) * scale + offset;

    float distance = min(1.0, abs(fragCoord.x - lightPos));
    float intensity = pow(cos(distance * 3.1415 / 2.0 ), 100.0);
    float d = intensity;
    if (d < 0.0) d = 0.0;
    float alpha = baseColor.a;
    return vec4(baseColor.r * d * alpha, baseColor.g * d * alpha, baseColor.b * d * alpha, 0.0);
}

void main() {
    vec2 fragCoord = inputs.position.xy;

    vec4 light0 = calculateLight(fragCoord, vec4(1.0, 1.0, 1.0, 1.0), 4.0, -1.41);
    vec4 light1 = calculateLight(fragCoord, vec4(1.0, 0.0, 0.0, 1.0), 4.1, -1.43);
    vec4 light2 = calculateLight(fragCoord, vec4(0.0, 0.0, 1.0, 1.0), 3.9, -1.38);

    float intensity = 0.2;
    vec4 col = inputs.color;

    vec4 fragColor = (vec4(col.r*intensity, col.g*intensity, col.b*intensity, col.a) + light0 + light1 + light2) * texture(iTexture, inputs.textureCoord);
    oColor = fragColor;
}
