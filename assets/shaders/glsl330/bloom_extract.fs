#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0; // The source render texture
uniform float threshold;    // Brightness threshold (usually 0.7 - 0.8)

out vec4 finalColor;

void main() {
    vec4 texColor = texture(texture0, fragTexCoord);
    
    // Relative luminance formula (ITU-R BT.709)
    float luminance = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    if (luminance > threshold) {
        // Keep the original colors for bright spots
        finalColor = vec4(texColor.rgb, 1.0);
    } else {
        // Discard or draw black for dark areas
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
