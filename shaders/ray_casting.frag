#version 330 core

uniform sampler2D slice_data;
uniform sampler3D volume_data;
uniform sampler3D border_data;
uniform int N_x;
uniform int N_y;
uniform int N_z;
uniform float slice_thickness;
uniform vec3 P_screen;
uniform vec3 v_width;
uniform vec3 v_height;
uniform vec3 v_normal;
uniform float window_level;
uniform float window_width;
uniform bool mode;
uniform bool border_visible;
uniform bool axial_visible;
uniform bool sagittal_visible;
uniform bool coronal_visible;

in vec3 vert_out;
out vec3 color;

bool is_penetrated(float x, float y, float z)
{	
	int t = 4;
	if (t < x && x < N_x - t && t < y && y < N_y - t &&	t * slice_thickness  < z && z < (N_z - t) * slice_thickness)
		return true;

	return false;
}

vec4 get_ray_penetrating_point(vec3 start)
{
	vec3 temp, result;
	bool found = false;
	float a, b, c, x, y, z, d;
	a = v_normal.x;
	b = v_normal.y;
	c = v_normal.z;
		
	if (a != 0) {
		x = 0;
		y = start.y + (x - start.x) * b / a;
		z = start.z + (x - start.x) * c / a;
		if (0 <= y && y <= N_y - 1 && 0 <= z && z <= N_z * slice_thickness - 1) {
			temp = vec3(x, y, z) - start;

			if (!found) {
				found = true;
				result = temp;
				d = length(temp);
			}
			else if (d > length(temp))
				result = temp;
			else
				d = length(temp);
		}
		
		x = N_x - 1;
		y = start.y + (x - start.x) * b / a;
		z = start.z + (x - start.x) * c / a;		
		if (0 <= y && y <= N_y - 1 && 0 <= z && z <= N_z * slice_thickness - 1) {
			temp = vec3(x, y, z) - start;

			if (!found) {
				found = true;
				result = temp;
				d = length(temp);
			}
			else if (d > length(temp))
				result = temp;
			else
				d = length(temp);
		}
	}

	if (b != 0) {
		y = 0;
		x = start.x + (y - start.y) * a / b;
		z = start.z + (y - start.y) * c / b;
		if (0 <= x && x <= N_x - 1 && 0 <= z && z <= N_z * slice_thickness - 1) {
			temp = vec3(x, y, z) - start;
			
			if (!found) {
				found = true;
				result = temp;
				d = length(temp);
			}
			else if (d > length(temp))
				result = temp;
			else
				d = length(temp);
		}

		y = N_y - 1;
		x = start.x + (y - start.y) * a / b;
		z = start.z + (y - start.y) * c / b;
		if (0 <= x && x <= N_x - 1 && 0 <= z && z <= N_z * slice_thickness - 1) {
			temp = vec3(x, y, z) - start;
			
			if (!found) {
				found = true;                                                                
				result = temp;
				d = length(temp);
			}
			else if (d > length(temp))
				result = temp;
			else
				d = length(temp);
		}
	}

	if (c != 0) {
		z = 0;
		x = start.x + (z - start.z) * a / c;
		y = start.y + (z - start.z) * b / c;
		if (0 <= x && x <= N_x - 1 && 0 <= y && y <= N_y - 1) {
			temp = vec3(x, y, z) - start;
			
			if (!found) {
				found = true;
				result = temp;
				d = length(temp);
			}
			else if (d > length(temp))
				result = temp;
			else
				d = length(temp);
		}

		z = slice_thickness * N_z - 1;
		x = start.x + (z - start.z) * a / c;
		y = start.y + (z - start.z) * b / c;
		if (0 <= x && x <= N_x - 1 && 0 <= y && y <= N_y - 1) {
			temp = vec3(x, y, z) - start;
			
			if (!found) {
				found = true;
				result = temp;
				d = length(temp);
			}
			else if (d > length(temp))
				result = temp;
			else
				d = length(temp);
		}
	}

	if (!found)
		return vec4(-1.0f, -1.0f, -1.0f, 0.0f);	

	return vec4(start + result, d);
}

void main()
{	
	vec3 start = P_screen + v_width * vert_out.x + v_height * vert_out.y;
	vec4 penetrating_point = get_ray_penetrating_point(start);
	vec3 depth = texture(slice_data, vec2(vert_out.x / 895, vert_out.y / 511)).xyz * 32768;	


	float red = 0.0f;
	float green = 0.0f;
	float blue = 0.0f;
	float slice_opacity = 0.5f;
	vec3 cur = penetrating_point.xyz;
	float accumulated_opacity = 1.0f;

	float start_len;
	if (cur.x < 0.0f) // no penetrating point
		start_len = N_x * N_y * N_z;
	else
		start_len = length(cur - start);

	if (!mode) {
		if (axial_visible && 1 < depth.z && depth.z < start_len) {
			red = slice_opacity;
			accumulated_opacity *= 1 - slice_opacity;

			if (sagittal_visible && 1 < depth.x && depth.x < depth.z)
				red *= 1 - slice_opacity;
			if (coronal_visible && 1 < depth.y && depth.y < depth.z)
				red *= 1 - slice_opacity;
		}
				
		if (sagittal_visible && 1 < depth.x && depth.x < start_len) {
			green = slice_opacity;
			accumulated_opacity *= 1 - slice_opacity;

			if (axial_visible && 1 < depth.z && depth.z < depth.x)
				green *= 1 - slice_opacity;
			if (coronal_visible && 1 < depth.y && depth.y < depth.x)
				green *= 1 - slice_opacity;
		}
				
		if (coronal_visible && 1 < depth.y && depth.y < start_len) {
			blue = slice_opacity;
			accumulated_opacity *= 1 - slice_opacity;

			if (axial_visible && 1 < depth.z && depth.z < depth.y)
				blue *= 1 - slice_opacity;
			if (sagittal_visible && 1 < depth.x && depth.x < depth.y)
				blue *= 1 - slice_opacity;
		}
	}

	vec3 accumulated_intensity = vec3(red, green, blue);
	if (cur.x < 0.0f) // no penetrating point
	{
		color = vec3(accumulated_intensity);
		return;
	}

	float cur_intensity, cur_opacity, cur_shadowed;
	float max_intensity = -1.0F;	
		
	int border_type = 0;
	bool penetrated = false;	
	bool touched = false;	
	float dx, dy, dz;

	while(true)
	{		
		if (length(cur - start) > penetrating_point.w) {
			if (mode)
				break;

			if ((length(cur - start) > depth.z || !axial_visible) && (length(cur - start) > depth.x || !sagittal_visible) && (length(cur - start) > depth.y || !coronal_visible))			
				break;
		}		

		if (!penetrated)
			penetrated = is_penetrated(cur.x, cur.y, cur.z);

		float border_value = texture(border_data, vec3(cur.x / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness))).x;
		if (border_visible && border_value > 0.9f)
		{
			if (!penetrated)
				border_type = 1;
			else if (!touched)
				border_type = 2;

			break;
		}
		
		cur_intensity = texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness))).x;				

		cur_opacity = (cur_intensity - window_level + window_width / 2) / window_width;
		if (cur_opacity < 0.0f)
			cur_opacity = 0.0f;
		if (cur_opacity > 1.0f)
			cur_opacity = 1.0f;
		if (cur_opacity > 0.0f)
			touched = true;

		//dx = (texture(volume_data, vec3((cur.x + 1) / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness))).x - texture(volume_data, vec3((cur.x - 1) / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness))).x) / 2;
		//dy = (texture(volume_data, vec3(cur.x / N_x, (cur.y + 1) / N_y, cur.z / (N_z * slice_thickness))).x - texture(volume_data, vec3(cur.x / N_x, (cur.y - 1) / N_y, cur.z / (N_z * slice_thickness))).x) / 2;
		//dz = (texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, (cur.z + slice_thickness) / (N_z * slice_thickness))).x - texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, (cur.z - slice_thickness) / (N_z * slice_thickness))).x) / (2 * slice_thickness);
		dx = (texture(volume_data, vec3((cur.x + 0.5) / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness))).x - texture(volume_data, vec3((cur.x - 0.5) / N_x, cur.y / N_y, cur.z / (N_z * slice_thickness))).x);
		dy = (texture(volume_data, vec3(cur.x / N_x, (cur.y + 0.5) / N_y, cur.z / (N_z * slice_thickness))).x - texture(volume_data, vec3(cur.x / N_x, (cur.y - 0.5) / N_y, cur.z / (N_z * slice_thickness))).x);
		dz = (texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, (cur.z + slice_thickness * 0.5) / (N_z * slice_thickness))).x - texture(volume_data, vec3(cur.x / N_x, cur.y / N_y, (cur.z - 0.5 * slice_thickness) / (N_z * slice_thickness))).x) / slice_thickness;
		cur_shadowed = clamp(0.1f + 0.9f * abs(dot(normalize(vec3(dx, dy, dz)), normalize(v_normal))), 0.0f, 1.0f);
		
		if (!mode)
		{			
			if (axial_visible) {
				if (length(cur - start) < depth.z) {
					if (length(cur - start + v_normal) >= depth.z) {
						accumulated_intensity += accumulated_opacity * slice_opacity * vec3(1.0f, 0.0f, 0.0f);
						accumulated_opacity *= 1.0f - slice_opacity;
					}
				}
			}

			if (sagittal_visible) {
				if (length(cur - start) < depth.x) {
					if (length(cur - start + v_normal) >= depth.x) {
						accumulated_intensity += accumulated_opacity * slice_opacity * vec3(0.0f, 1.0f, 0.0f);
						accumulated_opacity *= 1.0f - slice_opacity;
					}
				}
			}
			
			if (coronal_visible) {
				if (length(cur - start) < depth.y) {
					if (length(cur - start + v_normal) >= depth.y) {
						accumulated_intensity += accumulated_opacity * slice_opacity * vec3(0.0f, 0.0f, 1.0f);
						accumulated_opacity *= 1.0f - slice_opacity;
					}
				}
			}

			accumulated_intensity += accumulated_opacity * cur_opacity * cur_shadowed * vec3(1.0f, 1.0f, 1.0f);
			accumulated_opacity *= 1.0f - cur_opacity;

			if(accumulated_opacity < 0.001f)
				break;
		}
		else
		{
			if (max_intensity < cur_opacity)
				max_intensity = cur_opacity;
			if (max_intensity >= 1.0f)
				break;
		}

		cur = cur + v_normal;
	}

	if (!mode) {
		if (border_type == 1)
			accumulated_intensity += accumulated_opacity * vec3(1.0f, 0.0f, 1.0f);
		else if (border_type == 2)
			accumulated_intensity += accumulated_opacity * vec3(0.3f, 0.3f, 0.0f);

		color = accumulated_intensity;
	}
	else {
		if (border_type == 1)
			color = vec3(1.0f, 0.0f, 1.0f);
		else if (border_type == 2)
			color = vec3(0.3f, 0.3f, 0.0f);
		else
			color = vec3(max_intensity, max_intensity, max_intensity);	
	}
}

