
attribute vec3 vertex;
//attribute vec3 normal;
//attribute vec3 color;

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;

varying vec3 vert;
varying vec3 vertNormal;
varying vec3 vertColor;

void main() {
	vert = vertex;
	vertNormal = normalMatrix * vec3(0,-1,0);
	vertColor = vec3(0,1,0);
	gl_Position = projMatrix * mvMatrix * vec4(vertex, 1.0);
}