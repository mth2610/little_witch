#version 100
precision highp float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform int horizontal;
uniform vec2 renderSize; // Size of the texture being blurred

void main() {
    float weight[5];
    weight[0] = 0.227027;
    weight[1] = 0.1945946;
    weight[2] = 0.1216216;
    weight[3] = 0.054054;
    weight[4] = 0.016216;

    vec2 tex_offset = 1.0 / renderSize;
    vec3 result = texture2D(texture0, fragTexCoord).rgb * weight[0];
    
    if (horizontal != 0) {
        for (int i = 1; i < 5; ++i) {
            result += texture2D(texture0, fragTexCoord + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture2D(texture0, fragTexCoord - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture2D(texture0, fragTexCoord + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
            result += texture2D(texture0, fragTexCoord - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
        }
    }
    
    gl_FragColor = vec4(result, 1.0);
}
