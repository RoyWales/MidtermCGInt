#version 420

layout(binding = 0) uniform sampler2D uDisplay;
layout(binding = 1) uniform sampler2D uBloom;

in vec2 TexCoord;

out vec4 FragColor;

void main() {

	vec4 color1 = texture(uDisplay, TexCoord);
	vec4 color2 = texture(uBloom, TexCoord);

	FragColor = 1.0 - (1.0 - color1) * (1.0 - color2);

}

