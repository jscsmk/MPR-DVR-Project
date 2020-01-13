#version 330 core

uniform sampler2D slice_data;
uniform sampler3D volume_data;
//uniform sampler3D border_data;
uniform sampler1D octree_smin_data;
uniform sampler1D octree_smax_data;
uniform int N_x;
uniform int N_y;
uniform int N_z;
uniform int octree_max_depth;
uniform int octree_length;
uniform float slice_thickness;
uniform vec3 P_screen;
uniform vec3 v_width;
uniform vec3 v_height;
uniform vec3 v_normal;
uniform float window_level;
uniform float window_width;
uniform bool mode; // true = MIP, False = OTF
uniform bool skipping; // true = empty-space skipping, False = none
uniform bool border_visible;
uniform bool axial_visible;
uniform bool sagittal_visible;
uniform bool coronal_visible;

in vec3 vert_out;
out vec3 color;

float small_d, max_intensity, accumulated_opacity, slice_opacity;
vec3 accumulated_intensity, ray_start;
vec3 slice_color_list[3];
float slice_depth_list[3];
bool slice_visible_list[3];


bool is_penetrated(float x, float y, float z)
{
	int t = 4;
	if (t < x && x < N_x - t && t < y && y < N_y - t &&	t * slice_thickness  < z && z < (N_z - t) * slice_thickness)
		return true;

	return false;
}


void get_t_front_back(in float x_min, in float x_max, in float y_min, in float y_max, in float z_min, in float z_max, out float t0_x, out float t0_y, out float t0_z, out float t1_x, out float t1_y, out float t1_z)
{
	float a, b, c, temp;
	t0_x = -1.0 / 0.0;
	t0_y = -1.0 / 0.0;
	t0_z = -1.0 / 0.0;
	t1_x = 1.0 / 0.0;
	t1_y = 1.0 / 0.0;
	t1_z = 1.0 / 0.0;
	a = v_normal.x;
	b = v_normal.y;
	c = v_normal.z;

	if (a < 0) {
		temp = x_min;
		x_min = x_max;
		x_max = temp;
	}
	if (b < 0) {
		temp = y_min;
		y_min = y_max;
		y_max = temp;
	}
	if (c < 0) {
		temp = z_min;
		z_min = z_max;
		z_max = temp;
	}

	if (a != 0) {
		t0_x = (x_min - ray_start.x) / a;
		t1_x = (x_max - ray_start.x) / a;
	}
	if (b != 0) {
		t0_y = (y_min - ray_start.y) / b;
		t1_y = (y_max - ray_start.y) / b;
	}
	if (c != 0) {
		t0_z = (z_min - ray_start.z) / c;
		t1_z = (z_max - ray_start.z) / c;
	}
}


int get_first_child_idx(int idx, int depth)
{
	int i = 0, j = 1;
	int sib_idx = idx;
	int first_child_idx = idx;
	while (i < depth) {
		sib_idx -= j;
		j *= 8;
		i++;
	}

	first_child_idx += int(pow(8, depth)) + 7 * sib_idx;
	return first_child_idx;
}


void sample_through(in float t0, in float t1)
{
	vec3 cur = P_screen + v_width * vert_out.x + v_height * vert_out.y + t0 * v_normal;
	float cur_t = t0;
	float cur_intensity, cur_opacity, dx, dy, dz, cur_shadowed;

	while (true) {

		// stop when reached end
		if (cur_t > t1)
			break;

		// sample from data & calculate shadow
		cur_intensity = texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness))).x;
		cur_opacity = clamp((cur_intensity - window_level + window_width / 2) / window_width, 0.0f, 1.0f);
		dx = texture(volume_data, vec3(cur.x / N_x + small_d, cur.y / N_y, cur.z / (N_z * slice_thickness))).x - texture(volume_data, vec3(cur.x / N_x - small_d, cur.y / N_y, cur.z / (N_z * slice_thickness))).x;
		dy = texture(volume_data, vec3(cur.x / N_x, cur.y / N_y + small_d, cur.z / (N_z * slice_thickness))).x - texture(volume_data, vec3(cur.x / N_x, cur.y / N_y - small_d, cur.z / (N_z * slice_thickness))).x;
		dz = texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness) + small_d)).x - texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness) - small_d)).x;
		cur_shadowed = clamp(0.1f + 0.9f * abs(dot(normalize(vec3(dx, dy, dz) / (2 * small_d)), normalize(v_normal))), 0.0f, 1.0f);

		if (!mode) // OTF mode
		{
			// blend slice plane if passed through
			for (int i = 0; i < 3; i++) {
				if (slice_visible_list[i]) {
					if (slice_depth_list[i] <= cur_t) {
						accumulated_intensity += accumulated_opacity * slice_opacity * slice_color_list[i];
						accumulated_opacity *= 1.0f - slice_opacity;
						slice_visible_list[i] = false;
					}
				}
			}

			// accumulate intensity & opacity
			accumulated_intensity += accumulated_opacity * cur_opacity * cur_shadowed * vec3(1.0f, 1.0f, 1.0f);
			accumulated_opacity *= 1.0f - cur_opacity;

			// early ray termination
			if (accumulated_opacity < 0.001f)
				break;
		}
		else // MIP mode
		{
			max_intensity = max(max_intensity, cur_opacity);
			if (max_intensity >= 1.0f)
				break;
		}

		// go to next sampling point
		cur = cur + v_normal;
		cur_t += length(v_normal);
	}
}


void main()
{
	// set initial values
	ray_start = P_screen + v_width * vert_out.x + v_height * vert_out.y;
	small_d = 0.5f / max(max(N_x, N_y), N_z * slice_thickness);
	max_intensity = 0.0f;
	accumulated_opacity = 1.0f;
	accumulated_intensity = vec3(0.0f, 0.0f, 0.0f);
	slice_opacity = 0.5f;

	// add slice information to list, sort by depth
	vec3 slice_depth = texture(slice_data, vec2(vert_out.x / 895, vert_out.y / 511)).xyz * 32768;
	slice_depth_list[0] = slice_depth.z;
	slice_depth_list[1] = slice_depth.x;
	slice_depth_list[2] = slice_depth.y;
	slice_visible_list[0] = axial_visible && slice_depth.z > 1;
	slice_visible_list[1] = sagittal_visible && slice_depth.x > 1;
	slice_visible_list[2] = coronal_visible && slice_depth.y > 1;
	slice_color_list[0] = vec3(1.0f, 0.0f, 0.0f);
	slice_color_list[1] = vec3(0.0f, 1.0f, 0.0f);
	slice_color_list[2] = vec3(0.0f, 0.0f, 1.0f);

	for (int i = 0; i < 2; i++) {
		for (int j = i+1; j < 3; j++) {
			if (slice_depth_list[i] > slice_depth_list[j]) {
				float temp_f = slice_depth_list[i];
				slice_depth_list[i] = slice_depth_list[j];
				slice_depth_list[j] = temp_f;
				bool temp_b = slice_visible_list[i];
				slice_visible_list[i] = slice_visible_list[j];
				slice_visible_list[j] = temp_b;
				vec3 temp_v = slice_color_list[i];
				slice_color_list[i] = slice_color_list[j];
				slice_color_list[j] = temp_v;
			}
		}
	}

	// build octree stack
	int octree_idx_stack[4 * 3];
	vec3 octree_t0_stack[4 * 3];
	vec3 octree_t1_stack[4 * 3];
	int stack_top;
	float t_front, t_back, t0_x, t0_y, t0_z, t1_x, t1_y, t1_z;

	get_t_front_back(1, N_x-1, 0, N_y-1, 0, slice_thickness * N_z - 1, t0_x, t0_y, t0_z, t1_x, t1_y, t1_z);
	t_front = max(t0_x, max(t0_y, t0_z));
	t_back = min(t1_x, min(t1_y, t1_z));

	if (t_front <= t_back) {
		octree_idx_stack[0] = 0;
		octree_t0_stack[0] = vec3(t0_x, t0_y, t0_z);
		octree_t1_stack[0] = vec3(t1_x, t1_y, t1_z);
		stack_top = 0;
	}
	else // no intersection with 3d data
		stack_top = -1;

	//if (t_front < t_back)
	//	sample_through(t_front, t_back);

	// start sampling
	while (stack_top >= 0) {
		int cur_idx, cur_depth;
		vec3 cur_t0, cur_t1;
		float s_min, s_max;

		// pop from stack
		cur_idx = octree_idx_stack[stack_top];
		cur_t0 = octree_t0_stack[stack_top];
		cur_t1 = octree_t1_stack[stack_top];
		stack_top--;

		// lookup from octree texture
		float cur_idx_f = (float(cur_idx) + 0.5) / octree_length;
		//cur_depth = int((texture(octree_data, vec2(cur_idx_f, 0.1)).x * 65535 - 1) / 2);

		if (cur_idx == 0)
			cur_depth = 0;
		else if (cur_idx < 9)
			cur_depth = 1;
		else if (cur_idx < 73)
			cur_depth = 2;
		else
			cur_depth = 3;

		//s_min = 0.0f;
		//s_min = texture(octree_data, vec2(0.0, cur_idx_f)).x;
		//s_max = texture(octree_data, vec2(0.0f, cur_idx_f)).x;
		s_min = texture(octree_smin_data, cur_idx_f).x;
		s_max = texture(octree_smax_data, cur_idx_f).x;
		//s_max = 1.0;

		// if no match with transfer function, move on
		if (s_max < window_level - window_width / 2 || window_level + window_width / 2 < s_min)
			continue;

		if (cur_depth == octree_max_depth) {
			// if leaf node, sample through
			t_front = max(cur_t0.x, max(cur_t0.y, cur_t0.z));
			t_back = min(cur_t1.x, min(cur_t1.y, cur_t1.z));
			sample_through(t_front, t_back);
		}
		else {
			// if not leaf node, push child nodes that ray crosses to stack, in reverse order
			vec3 cur_tm;
			cur_tm.x = isinf(cur_t0.x) ? -1.0 / 0.0 : (cur_t0.x + cur_t1.x) / 2;
			cur_tm.y = isinf(cur_t0.y) ? -1.0 / 0.0 : (cur_t0.y + cur_t1.y) / 2;
			cur_tm.z = isinf(cur_t0.z) ? -1.0 / 0.0 : (cur_t0.z + cur_t1.z) / 2;

			// check if ray crosses for all 8 child nodes
			float child_t_front_list[8];
			vec3 child_t0_list[8], child_t1_list[8];
			int child_idx_list[8];
			int child_cross_num = 0;

			child_t0_list[0] = vec3(cur_t0.x, cur_t0.y, cur_t0.z);
			child_t0_list[1] = vec3(cur_tm.x, cur_t0.y, cur_t0.z);
			child_t0_list[2] = vec3(cur_t0.x, cur_tm.y, cur_t0.z);
			child_t0_list[3] = vec3(cur_tm.x, cur_tm.y, cur_t0.z);
			child_t0_list[4] = vec3(cur_t0.x, cur_t0.y, cur_tm.z);
			child_t0_list[5] = vec3(cur_tm.x, cur_t0.y, cur_tm.z);
			child_t0_list[6] = vec3(cur_t0.x, cur_tm.y, cur_tm.z);
			child_t0_list[7] = vec3(cur_tm.x, cur_tm.y, cur_tm.z);

			if (isinf(cur_tm.x))
				cur_tm.x = 1.0 / 0.0;
			if (isinf(cur_tm.y))
				cur_tm.y = 1.0 / 0.0;
			if (isinf(cur_tm.z))
				cur_tm.z = 1.0 / 0.0;

			child_t1_list[0] = vec3(cur_tm.x, cur_tm.y, cur_tm.z);
			child_t1_list[1] = vec3(cur_t1.x, cur_tm.y, cur_tm.z);
			child_t1_list[2] = vec3(cur_tm.x, cur_t1.y, cur_tm.z);
			child_t1_list[3] = vec3(cur_t1.x, cur_t1.y, cur_tm.z);
			child_t1_list[4] = vec3(cur_tm.x, cur_tm.y, cur_t1.z);
			child_t1_list[5] = vec3(cur_t1.x, cur_tm.y, cur_t1.z);
			child_t1_list[6] = vec3(cur_tm.x, cur_t1.y, cur_t1.z);
			child_t1_list[7] = vec3(cur_t1.x, cur_t1.y, cur_t1.z);

			for (int i = 0; i < 8; i++) {
				t_front = max(child_t0_list[i].x, max(child_t0_list[i].y, child_t0_list[i].z));
				t_back = min(child_t1_list[i].x, min(child_t1_list[i].y, child_t1_list[i].z));
				if (t_front <= t_back) {
					child_t_front_list[child_cross_num] = t_front;
					child_idx_list[child_cross_num] = i;
					child_cross_num++;
				}
			}

			// sort
			for (int i = 0; i < child_cross_num - 1; i++) {
				for (int j = i+1; j < child_cross_num; j++) {
					if (child_t_front_list[i] < child_t_front_list[j]) {
						float temp_t = child_t_front_list[i];
						child_t_front_list[i] = child_t_front_list[j];
						child_t_front_list[j] = temp_t;
						int temp_i = child_idx_list[i];
						child_idx_list[i] = child_idx_list[j];
						child_idx_list[j] = temp_i;
					}
				}
			}

			// push to stack
			for (int i = 0; i < child_cross_num; i++) {
				stack_top++;

				int child_idx = child_idx_list[i];
				octree_t0_stack[stack_top] = child_t0_list[child_idx];
				octree_t1_stack[stack_top] = child_t1_list[child_idx];

				if (v_normal.x < 0)
					child_idx ^= 1;
				if (v_normal.y < 0)
					child_idx ^= 2;
				if (v_normal.z < 0)
					child_idx ^= 4;

				octree_idx_stack[stack_top] = get_first_child_idx(cur_idx, cur_depth) + child_idx;
			}
		}
	}


	// blend slice plane if not passed through yet
	for (int i = 0; i < 3; i++) {
		if (slice_visible_list[i]) {
			accumulated_intensity += accumulated_opacity * slice_opacity * slice_color_list[i];
			accumulated_opacity *= 1.0f - slice_opacity;
		}
	}

	// determine final color value
	// TODO: deal with border lines
	if (!mode) { // OTF mode
		//if (border_type == 1)
		//	accumulated_intensity += accumulated_opacity * vec3(1.0f, 0.0f, 1.0f);
		//else if (border_type == 2)
		//	accumulated_intensity += accumulated_opacity * vec3(0.3f, 0.0f, 0.3f);

		color = accumulated_intensity;
	}
	else { // MIP mode
		//if (border_type == 1)
		//	color = vec3(1.0f, 0.0f, 1.0f);
		//else if (border_type == 2)
		//	color = vec3(0.3f, 0.3f, 0.0f);
		//else
		color = vec3(max_intensity, max_intensity, max_intensity);
	}
}
