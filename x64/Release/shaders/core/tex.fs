#version 430

in vec2 pos;
in vec2 posPx;

layout (binding=0) uniform sampler2D tex;

out vec4 FragColor;

void main() {
	FragColor = texture(tex, pos);
}