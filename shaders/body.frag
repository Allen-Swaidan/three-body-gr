#version 450 core

in vec3 FragPos;
in vec3 Normal;

uniform vec4 bodyColor;
uniform vec3 viewPos;

out vec4 FragColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 norm = normalize(Normal);

    // Ambient
    float ambient = 0.15;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);

    // Rim light for glow effect
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = pow(rim, 3.0) * 0.5;

    vec3 result = bodyColor.rgb * (ambient + diff * 0.7 + spec * 0.4) + bodyColor.rgb * rim;

    // Slight bloom
    result += bodyColor.rgb * 0.1;

    FragColor = vec4(result, bodyColor.a);
}
