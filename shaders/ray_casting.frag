#version 330 core

uniform sampler2D slice_data;
uniform sampler3D volume_data;
//uniform sampler3D border_data;
//uniform sampler1D octree_smin_data;
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
float a, b, c;


bool is_penetrated(float x, float y, float z)
{
	int t = 4;
	if (t < x && x < N_x - t && t < y && y < N_y - t &&	t * slice_thickness  < z && z < (N_z - t) * slice_thickness)
		return true;

	return false;
}


void get_t_front_back(in float x_min, in float x_max, in float y_min, in float y_max, in float z_min, in float z_max, out float t0_x, out float t0_y, out float t0_z, out float t1_x, out float t1_y, out float t1_z)
{
	t0_x = (x_min - ray_start.x) / a;
	t1_x = (x_max - ray_start.x) / a;
	t0_y = (y_min - ray_start.y) / b;
	t1_y = (y_max - ray_start.y) / b;
	t0_z = (z_min - ray_start.z) / c;
	t1_z = (z_max - ray_start.z) / c;

	if (a == 0) {
		t0_x = -1.0 / 0.0;
		t1_x = 1.0 / 0.0;
		if (ray_start.x < x_min)
			t0_x = 1.0 / 0.0;
		if (x_max < ray_start.x)
			t0_x = -1.0 / 0.0;
	}
	if (b == 0) {
		t0_y = -1.0 / 0.0;
		t1_y = 1.0 / 0.0;
		if (ray_start.y < y_min)
			t0_y = 1.0 / 0.0;
		if (y_max < ray_start.x)
			t0_y = -1.0 / 0.0;
	}
	if (c == 0)	{
		t0_z = -1.0 / 0.0;
		t1_z = 1.0 / 0.0;
		if (ray_start.z < z_min)
			t0_z = 1.0 / 0.0;
		if (z_max < ray_start.x)
			t0_z = -1.0 / 0.0;
	}
}


void sample_through(in float t0, in float t1)
{
	vec3 cur = P_screen + v_width * vert_out.x + v_height * vert_out.y + t0 * v_normal;
	float cur_t = t0;
	float cur_intensity, cur_opacity, dx, dy, dz, cur_shadowed;

	while (cur_t < t1) { // stop when reached end
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

			// early ray termination
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
	a = v_normal.x;
	b = v_normal.y;
	c = v_normal.z;
	int get_next_sibling[24] = int[](1,2,4, 8,3,5, 3,8,6, 8,8,7, 5,6,8, 8,7,8, 7,8,8, 8,8,8);

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
	int octree_idx_stack[6];
	int octree_child_num_stack[6];
	bool octree_visited_stack[6];
	vec3 octree_t0_stack[6];
	vec3 octree_t1_stack[6];
	vec3 octree_min_xyz_stack[6];
	vec3 octree_max_xyz_stack[6];
	int stack_top = -1;
	int child_num_shifter = 0;
	float t_front, t_back, t0_x, t0_y, t0_z, t1_x, t1_y, t1_z;
	float x_min, x_max, y_min, y_max, z_min, z_max;
	x_min = 0;
	y_min = 0;
	z_min = 0;
	x_max = N_x - 1;
	y_max = N_y - 1;
	z_max = N_z * slice_thickness - 1;
	if (a < 0) {
		x_min = N_x - 1;
		x_max = 0;
		child_num_shifter += 1;
	}
	if (b < 0) {
		y_min = N_y - 1;
		y_max = 0;
		child_num_shifter += 2;
	}
	if (c < 0) {
		z_min = N_z * slice_thickness - 1;
		z_max = 0;
		child_num_shifter += 4;
	}

	// examine first node
	get_t_front_back(x_min, x_max, y_min, y_max, z_min, z_max, t0_x, t0_y, t0_z, t1_x, t1_y, t1_z);
	t_front = max(t0_x, max(t0_y, t0_z));
	t_back = min(t1_x, min(t1_y, t1_z));
	if (t_front <= t_back) { // ray intersects with data
		if (skipping) {
			octree_idx_stack[0] = 0;
			octree_child_num_stack[0] = 0;
			octree_t0_stack[0] = vec3(t0_x, t0_y, t0_z);
			octree_t1_stack[0] = vec3(t1_x, t1_y, t1_z);
			octree_min_xyz_stack[0] = vec3(x_min, y_min, z_min);
			octree_max_xyz_stack[0] = vec3(x_max, y_max, z_max);
			octree_visited_stack[0] = false;
			stack_top = 0;
		}
		else // no empty-space skipping
			sample_through(t_front, t_back);
	}

	// start push & pop stack
	while (stack_top >= 0) {
		// get top information from stack
		int cur_first_sibling_idx, cur_child_num, cur_depth, actual_child_num;
		vec3 cur_t0, cur_t1, cur_min_xyz, cur_max_xyz, cur_mid_xyz;
		float s_max;
		bool visited = octree_visited_stack[stack_top];
		cur_first_sibling_idx = octree_idx_stack[stack_top];
		cur_child_num = octree_child_num_stack[stack_top];
		cur_t0 = octree_t0_stack[stack_top];
		cur_t1 = octree_t1_stack[stack_top];
		cur_depth = stack_top;
		actual_child_num = cur_child_num ^ child_num_shifter;
		if (cur_depth == 0)
			actual_child_num = 0;

		// lookup from octree texture
		s_max = texture(octree_smax_data, ((cur_first_sibling_idx + actual_child_num + 0.5) / octree_length)).x;

		// decide to move to child or sibling
		if (s_max > window_level - window_width / 2 && cur_depth < octree_max_depth && !visited) {
			octree_visited_stack[stack_top] = true;

			// if not skip and not leaf node, push child node to stack, which ray crosses
			float tm_x, tm_y, tm_z;
			tm_x = (cur_t0.x + cur_t1.x) / 2;
			tm_y = (cur_t0.y + cur_t1.y) / 2;
			tm_z = (cur_t0.z + cur_t1.z) / 2;

			int next_child_num = 0;
			if (cur_t0.x > cur_t0.y && cur_t0.x > cur_t0.z) { // enters YZ plane
				if (cur_t0.x > tm_y)
					next_child_num |= 2;

				if (cur_t0.x > tm_z)
					next_child_num |= 4;
			}
			else if (cur_t0.y > cur_t0.z) { // enters XZ plane
				if (cur_t0.y > tm_x)
					next_child_num |= 1;

				if (cur_t0.y > tm_z)
					next_child_num |= 4;
			}
			else { // enters XY plane
				if (cur_t0.z > tm_x)
					next_child_num |= 1;

				if (cur_t0.z > tm_y)
					next_child_num |= 2;
			}

			stack_top++;
			octree_child_num_stack[stack_top] = next_child_num;
			octree_idx_stack[stack_top] = 8 * (cur_first_sibling_idx + actual_child_num) + 1;
			octree_visited_stack[stack_top] = false;

			float t0t1_x[3] = float[](cur_t0.x, tm_x, cur_t1.x);
			float t0t1_y[3] = float[](cur_t0.y, tm_y, cur_t1.y);
			float t0t1_z[3] = float[](cur_t0.z, tm_z, cur_t1.z);
			octree_t0_stack[stack_top] = vec3(
				t0t1_x[next_child_num & 1],
				t0t1_y[(next_child_num & 2) / 2],
				t0t1_z[(next_child_num & 4) / 4]
			);
			octree_t1_stack[stack_top] = vec3(
				t0t1_x[(next_child_num & 1) + 1],
				t0t1_y[((next_child_num & 2) / 2) + 1],
				t0t1_z[((next_child_num & 4) / 4) + 1]
			);
		}
		else {
			if (cur_depth == octree_max_depth && s_max > window_level - window_width / 2) {
				// if leaf node and not skip, sample through
				t_front = max(cur_t0.x, max(cur_t0.y, cur_t0.z));
				t_back = min(cur_t1.x, min(cur_t1.y, cur_t1.z));
				sample_through(t_front, t_back);

				// early ray termination
				if (accumulated_opacity < 0.001f || max_intensity >= 1.0f)
					break;
			}

			// find next sibling node, push to stack
			if (cur_depth == 0)
				break;

			int exit_plane, next_sibling;
			if (cur_t1.x < cur_t1.y && cur_t1.x < cur_t1.z)
				exit_plane = 0;
			else if (cur_t1.y < cur_t1.z)
				exit_plane = 1;
			else
				exit_plane = 2;

			next_sibling = get_next_sibling[3 * cur_child_num + exit_plane];
			if (next_sibling == 8) {
				stack_top--;
			}
			else {
				octree_child_num_stack[stack_top] = next_sibling;
				octree_visited_stack[stack_top] = false;

				float choice_t0_x[2] = float[](cur_t1.x, cur_t0.x);
				float choice_t0_y[2] = float[](cur_t0.y, cur_t1.y);
				float choice_t0_z[2] = float[](cur_t0.z, cur_t1.z);
				float choice_t1_x[2] = float[](2 * cur_t1.x - cur_t0.x, cur_t1.x);
				float choice_t1_y[2] = float[](cur_t1.y, 2 * cur_t1.y - cur_t0.y);
				float choice_t1_z[2] = float[](cur_t1.z, 2 * cur_t1.z - cur_t0.z);
				octree_t0_stack[stack_top] = vec3(
					choice_t0_x[(exit_plane + 1) / 2],
					choice_t0_y[exit_plane & 1],
					choice_t0_z[exit_plane / 2]
				);
				octree_t1_stack[stack_top] = vec3(
					choice_t1_x[(exit_plane + 1) / 2],
					choice_t1_y[exit_plane & 1],
					choice_t1_z[exit_plane / 2]
				);
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
