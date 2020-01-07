#version 330 core

uniform mat4 projMatrix;
uniform mat4 mvMatrix;

in vec3 vert_in;
out vec3 vert_out;

void main()
{
	vert_out = vert_in;
	gl_Position = projMatrix * mvMatrix * vec4(vert_in, 1.0);
}