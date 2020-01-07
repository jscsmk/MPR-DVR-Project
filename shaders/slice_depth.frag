#version 330 core

uniform sampler1D P_1;
uniform sampler1D P_2;
uniform sampler1D P_3;
uniform int N;
uniform vec3 P_screen;
uniform vec3 ns;
uniform vec3 ws;
uniform vec3 hs;


uniform vec3 qx;
uniform vec3 nx;
uniform vec3 wx;
uniform vec3 hx;
uniform vec3 qy;
uniform vec3 ny;
uniform vec3 wy;
uniform vec3 hy;
uniform vec3 qz;
uniform vec3 nz;
uniform vec3 wz;
uniform vec3 hz;
uniform int L_x;
uniform int L_y;
uniform int L_z;


vec4 temp_output[7];
int count = 0;

in vec3 vert_out;
layout (location = 0) out vec4 color_0;
/*
layout (location = 1) out vec4 color_1;
layout (location = 2) out vec4 color_2;
layout (location = 3) out vec4 color_3;
layout (location = 4) out vec4 color_4;
layout (location = 5) out vec4 color_5;
layout (location = 6) out vec4 color_6;

void insert_in()
{

}
*/

void main()
{	
	
	vec3 cur = P_screen + ws * vert_out.x + hs * vert_out.y;
	float depth_x, depth_y, depth_z, pr;
	vec3 vec_from_q, pr_w, pr_h;		

	// x slice
	if (dot(ns, nx) == 0)
		depth_x = 0.0f;
	else {
		depth_x = (dot(qx, nx) - dot(cur, nx)) / dot(ns, nx);
		vec_from_q = cur + depth_x * ns - qx;
		pr = dot(wx, vec_from_q);
		pr_w = wx * pr;
		pr_h = vec_from_q - pr_w;

		// check range
		if (dot(pr_w, wx) < 0 || dot(pr_h, hx) < 0 || length(pr_w) > L_x * 7 / 4 || length(pr_h) > L_x)
			depth_x = 0.0f;
	}
	
	// y slice
	if (dot(ns, ny) == 0)
		depth_y = 0.0f;
	else {
		depth_y = (dot(qy, ny) - dot(cur, ny)) / dot(ns, ny);
		vec_from_q = cur + depth_y * ns - qy;
		pr = dot(wy, vec_from_q);
		pr_w = wy * pr;
		pr_h = vec_from_q - pr_w;

		// check range
		if (dot(pr_w, wy) < 0 || dot(pr_h, hy) < 0 || length(pr_w) > L_y * 7 / 4 || length(pr_h) > L_y)
			depth_y = 0.0f;
	}
	
	// z slice
	if (dot(ns, nz) == 0)
		depth_z = 0.0f;
	else {
		depth_z = (dot(qz, nz) - dot(cur, nz)) / dot(ns, nz);
		vec_from_q = cur + depth_z * ns - qz;
		pr = dot(wz, vec_from_q);
		pr_w = wz * pr;
		pr_h = vec_from_q - pr_w;
			
		// check range
		if (dot(pr_w, wz) < 0 || dot(pr_h, hz) < 0 || length(pr_w) > L_z * 7 / 4 || length(pr_h) > L_z)
			depth_z = 0.0f;
	}
	
	color_0 = vec4(depth_x, depth_y, depth_z, 0.0f) / 32768;
	
	/*
	vec3 cur = P_screen + ws * vert_out.x + hs * vert_out.y;

	for (int i = 0; i < N; i++) {

	}

	// final output
	color_0 = temp_output[0];
	color_1 = temp_output[1];
	color_2 = temp_output[2];
	color_3 = temp_output[3];
	color_4 = temp_output[4];
	color_5 = temp_output[5];
	color_6 = temp_output[6];
	*/
}
