#version 450 core

uniform vec4 traceColor;
out vec4 FragColor;

void main() {
    FragColor = traceColor;
}
