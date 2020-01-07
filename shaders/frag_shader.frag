
varying highp vec3 vert;
varying highp vec3 vertNormal;
varying highp vec3 vertColor;

uniform highp vec3 lightPos;
uniform sampler3D VolumeTex;

void main() {
	highp vec3 col = clamp(vertColor * 0.1 + vertColor * 0.9 * max(dot(normalize(vertNormal), normalize(lightPos - vert)), 0.0), 0.0, 1.0);	
	gl_FragColor = vec4(col, 1.0);
}