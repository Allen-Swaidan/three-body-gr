#version 450 core

in float Curvature;
in vec3 FragPos;

out vec4 FragColor;

void main() {
    // Color based on curvature intensity
    float intensity = clamp(Curvature * 0.3, 0.0, 1.0);

    // Deep blue to cyan to white gradient based on curvature
    vec3 lowColor = vec3(0.05, 0.1, 0.3);    // dark blue (flat space)
    vec3 midColor = vec3(0.0, 0.5, 0.8);     // cyan (moderate curvature)
    vec3 highColor = vec3(0.8, 0.9, 1.0);    // white (high curvature)

    vec3 color;
    if (intensity < 0.5) {
        color = mix(lowColor, midColor, intensity * 2.0);
    } else {
        color = mix(midColor, highColor, (intensity - 0.5) * 2.0);
    }

    float alpha = 0.3 + intensity * 0.5;
    FragColor = vec4(color, alpha);
}
