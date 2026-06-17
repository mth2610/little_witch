#version 100
precision highp float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0; // The source render texture
uniform float threshold;    // Brightness threshold

void main() {
    vec4 texColor = texture2D(texture0, fragTexCoord);
    
    // Relative luminance formula (ITU-R BT.709)
    float luminance = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    if (luminance > threshold) {
        gl_FragColor = vec4(texColor.rgb, 1.0);
    } else {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
