#include "data_cube.h"
#include <iostream>
#include <math.h>
#include <algorithm>
#include <tuple>
#include <QMatrix4x4>
#include <QVector>
#include <QVector4D>
#include <QVector3D>
#include <qopengl.h>
#include <qmath.h>
using namespace std;
#define PI 3.141592


DataCube::DataCube()
{
	x_border_visible = 0;
	y_border_visible = 0;
	z_border_visible = 0;
}

void DataCube::set_data(int *data, int x, int y, int z, int p, int a, int b, float t, int mi, int ma)
{
	data_3d = data;
	N_x = x;
	N_y = y;
	N_z = z;
	slice_pixel_num = p;
	rescale_slope = a;
	rescale_intercept = b;
	slice_thickness = t;
	pixel_min = mi;
	pixel_max = ma;

	N_max = max(max(x, y), (int)(z * t));

	m_trans_center.setToIdentity();
	m_trans_center.translate(QVector3D(-N_x / 2, -N_y / 2, -N_z * slice_thickness / 2));
	m_trans_center_inverse.setToIdentity();
	m_trans_center_inverse.translate(QVector3D(N_x / 2, N_y / 2, N_z * slice_thickness / 2));

	//slice = (int*)malloc(slice_pixel_num * slice_pixel_num * sizeof(int) * 7 / 4);

	init_MPR();
}

void DataCube::init_MPR()
{
	L_x = N_max;
	L_y = N_max;
	L_z = N_max;

	pixel_len_x = L_x / (slice_pixel_num - 1);
	pixel_len_y = pixel_len_x;
	pixel_len_z = pixel_len_x;

	P_axis = QVector3D(N_x / 2, N_y / 2, N_z * slice_thickness / 2);

	nx = QVector3D(1, 0, 0);
	ny = QVector3D(0, 1, 0);
	nz = QVector3D(0, 0, 1);

	//wx = QVector3D(0, 0, 1);
	//hx = QVector3D(0, 1, 0);
	wx = QVector3D(0, -1, 0);
	hx = QVector3D(0, 0, 1);
	wy = QVector3D(1, 0, 0);
	hy = QVector3D(0, 0, 1);
	wz = QVector3D(1, 0, 0);
	hz = QVector3D(0, 1, 0);

	qz = P_axis - QVector3D(L_z * 7 / 8, L_z / 2, 0);
	qx = P_axis + QVector3D(0, L_x * 7 / 8, -L_x / 2);
	qy = P_axis - QVector3D(L_y * 7 / 8, 0, L_y / 2);

	//r_x = 0;
	r_x = PI / 2;
	r_y = 0;
	r_z = 0;
}

tuple<int, int, int, float> DataCube::get_data_size()
{
	return { N_x, N_y, N_z, slice_thickness };
}
tuple<int, int, int, int, int> DataCube::get_pixel_info()
{
	return {slice_pixel_num, rescale_slope, rescale_intercept, pixel_min, pixel_max };
}
int* DataCube::get_raw_data()
{
	return data_3d;
}

void DataCube::toggle_border_line(int slice_type)
{
	if (slice_type == 0)
		z_border_visible = (z_border_visible + 1) % 2;
	else if (slice_type == 1)
		x_border_visible = (x_border_visible + 1) % 2;
	else
		y_border_visible = (y_border_visible + 1) % 2;
}

void DataCube::get_slice(int slice_type, int *slice_data)
{
	//int* slice = (int*)malloc(slice_pixel_num * slice_pixel_num * sizeof(int) * 7 / 4);
	float pl;
	QVector3D start, temp, w, h;

	if (slice_type == 0) { // z slice
		start = qz;
		w = wz;
		h = hz;
		pl = pixel_len_z;
	}
	else if (slice_type == 1) { // x slice
		start = qx;
		w = wx;
		h = hx;
		pl = pixel_len_x;
	}
	else { // y slice
		start = qy;
		w = wy;
		h = hy;
		pl = pixel_len_y;
	}

	w = pl * w;
	h = pl * h;

	for (int i = 0; i < slice_pixel_num; i++) {
		temp = start;

		for (int j = 0; j < slice_pixel_num * 7 / 4; j++) {
			slice_data[slice_pixel_num*i*7/4 + j] = trilinear_interpolation(slice_type, temp.x(), temp.y(), temp.z());
			temp = temp + w;
		}
		start = start + h;
	}

	//return slice;
}

tuple<int, int, float> DataCube::get_line_info(int slice_type)
{
	int line_x, line_y;
	float pr, pl, line_angle;
	QVector3D temp, w, h, pr_w, pr_h;

	if (slice_type == 0) { // z slice
		temp = P_axis - qz;
		w = wz;
		h = hz;
		pl = pixel_len_z;
		line_angle = r_z;
	}
	else if (slice_type == 1) { // x slice
		temp = P_axis - qx;
		w = wx;
		h = hx;
		pl = pixel_len_x;
		line_angle = r_x;
	}
	else { // y slice
		temp = P_axis - qy;
		w = wy;
		h = hy;
		pl = pixel_len_y;
		line_angle = r_y;
	}

	// get projection to w, h vector
	pr = QVector3D::dotProduct(w, temp);
	pr_w = w * pr;
	pr_h = temp - pr_w;

	// length of each projection is coord of line
	line_x = (int)(pr_w.length() / pl);
	line_y = (int)(pr_h.length() / pl);

	// check if negative
	if (QVector3D::dotProduct(pr_w, w) < 0)
		line_x *= -1;
	if (QVector3D::dotProduct(pr_h, h) < 0)
		line_y *= -1;

	return {line_x, line_y, line_angle};
}

int DataCube::trilinear_interpolation(int slice_type, float x, float y, float z)
{
	z /= slice_thickness;

	// see https://darkpgmr.tistory.com/117
	int draw_border = 0;
	float pl = 1;
	if (slice_type == 0) {
		draw_border = z_border_visible;
		pl = pixel_len_z;
	}
	else if (slice_type == 1) {
		draw_border = x_border_visible;
		pl = pixel_len_x;
	}
	else if (slice_type == 2) {
		draw_border = y_border_visible;
		pl = pixel_len_y;
	}

	if (x < -2*pl || y < -2*pl || z < -2*pl || x >= N_x - 1 + 2*pl || y >= N_y - 1 + 2*pl || z >= N_z - 1 + 2*pl) {
		// if out of range, return min val
		return pixel_min;
	}

	if (x < 0 || y < 0 || z < 0 || x >= N_x - 1 || y >= N_y - 1 || z >= N_z - 1) {
		// if border, return -1
		if (draw_border == 1)
			return pixel_min-1;

		return pixel_min;
	}

	int fA, fB, fC, fD, fE, fF, fG, fH;
	float fM, fN, fU, fV, fR, fS, fP;
	float a, b;

	int x_floor = floor(x);
	int y_floor = floor(y);
	int z_floor = floor(z);
	int x_ceil = x_floor + 1;
	int y_ceil = y_floor + 1;
	int z_ceil = z_floor + 1;

	// get data values
	fA = data_3d[N_x*N_y*z_floor + N_x*y_floor + x_floor];
	fB = data_3d[N_x*N_y*z_floor + N_x*y_ceil + x_floor];
	fC = data_3d[N_x*N_y*z_floor + N_x*y_ceil + x_ceil];
	fD = data_3d[N_x*N_y*z_floor + N_x*y_floor + x_ceil];
	fE = data_3d[N_x*N_y*z_ceil + N_x*y_floor + x_floor];
	fF = data_3d[N_x*N_y*z_ceil + N_x*y_ceil + x_floor];
	fG = data_3d[N_x*N_y*z_ceil + N_x*y_ceil + x_ceil];
	fH = data_3d[N_x*N_y*z_ceil + N_x*y_floor + x_ceil];

	// interpolation (x-axis)
	a = x - x_floor;
	b = 1 - a;
	fM = b*fA + a*fD;
	fN = b*fB + a*fC;
	fU = b*fE + a*fH;
	fV = b*fF + a*fG;

	// interpolation (y-axis)
	a = y - y_floor;
	b = 1 - a;
	fR = b*fM + a*fN;
	fS = b*fU + a*fV;

	// interpolation (z-axis)
	a = z - z_floor;
	b = 1 - a;
	fP = b*fR + a*fS;

	return (int)fP;
}

tuple<int, int, int> DataCube::closest_neighbor(float x, float y, float z)
{
	return { floor(x + 0.5), floor(y + 0.5), floor(z + 0.5) };
}

tuple<int, int, int, int> DataCube::get_coord(int slice_type, int m_x, int m_y)
{
	QVector3D point;
	int cn_x, cn_y, cn_z, pixel_v;

	if (slice_type == 0) // z slice
		point = qz + pixel_len_z * (m_x * wz + m_y * hz);
	else if (slice_type == 1) // x slice
		point = qx + pixel_len_x * (m_x * wx + m_y * hx);
	else // y slice
		point = qy + pixel_len_y * (m_x * wy + m_y * hy);

	tie(cn_x, cn_y, cn_z) = closest_neighbor(point.x(), point.y(), point.z());
	pixel_v = trilinear_interpolation(-1, cn_x, cn_y, cn_z);

	return {cn_x, cn_y, cn_z / slice_thickness, pixel_v};
}

int DataCube::move_slice(int from, int target, float distance)
{
	QVector3D d, new_p;

	if (target == 0) // move z slice
		d = nz;
	else if (target == 1) // move x slice
		d = nx;
	else // move z slice
		d = ny;

	if (from == 0) // from z slice
		d = d * distance * pixel_len_z;
	else if (from == 1) // from x slice
		d = d * distance * pixel_len_x;
	else // from y slice
		d = d * distance * pixel_len_y;

	new_p = P_axis + d;
	if (new_p.x() < 0 || N_x - 1 < new_p.x() ||
		new_p.y() < 0 || N_y - 1 < new_p.y() ||
		new_p.z() < 0 || (N_z - 1) * slice_thickness < new_p.z())
		return 0;

	P_axis = new_p;
	if (target == 0) // move z slice
		qz = qz + d;
	else if (target == 1) // move x slice
		qx = qx + d;
	else // move y slice
		qy = qy + d;

	return 1;
}

int DataCube::rotate_slice(int slice_type, float a)
{
	QMatrix4x4 m_rotate, m_trans_axis, m_trans_axis_inverse;

	m_rotate.setToIdentity();
	if (slice_type == 0)
		m_rotate.rotate(a * 180 / PI, nz);
	else if (slice_type == 1)
		m_rotate.rotate(a * 180 / PI, -nx);
	else
		m_rotate.rotate(a * 180 / PI, -ny);

	m_trans_axis.setToIdentity();
	m_trans_axis.translate(-P_axis);
	m_trans_axis_inverse.setToIdentity();
	m_trans_axis_inverse.translate(P_axis);

	if (slice_type == 0) {
		r_z += a;
		if (r_z >= 2 * PI)
			r_z -= 2 * PI;
		else if (r_z < 0)
			r_z += 2 * PI;

		qx = (m_trans_axis_inverse * m_rotate * m_trans_axis * QVector4D(qx, 1)).toVector3D();
		nx = (m_rotate * QVector4D(nx, 1)).toVector3D();
		wx = (m_rotate * QVector4D(wx, 1)).toVector3D();
		hx = (m_rotate * QVector4D(hx, 1)).toVector3D();

		qy = (m_trans_axis_inverse * m_rotate * m_trans_axis * QVector4D(qy, 1)).toVector3D();
		ny = (m_rotate * QVector4D(ny, 1)).toVector3D();
		wy = (m_rotate * QVector4D(wy, 1)).toVector3D();
		hy = (m_rotate * QVector4D(hy, 1)).toVector3D();

	}
	else if (slice_type == 1) {
		r_x += a;
		if (r_x >= 2 * PI)
			r_x -= 2 * PI;
		else if (r_x < 0)
			r_x += 2 * PI;

		qz = (m_trans_axis_inverse * m_rotate * m_trans_axis * QVector4D(qz, 1)).toVector3D();
		nz = (m_rotate * QVector4D(nz, 1)).toVector3D();
		wz = (m_rotate * QVector4D(wz, 1)).toVector3D();
		hz = (m_rotate * QVector4D(hz, 1)).toVector3D();

		qy = (m_trans_axis_inverse * m_rotate * m_trans_axis * QVector4D(qy, 1)).toVector3D();
		ny = (m_rotate * QVector4D(ny, 1)).toVector3D();
		wy = (m_rotate * QVector4D(wy, 1)).toVector3D();
		hy = (m_rotate * QVector4D(hy, 1)).toVector3D();
	}
	else {
		r_y += a;
		if (r_y >= 2 * PI)
			r_y -= 2 * PI;
		else if (r_y < 0)
			r_y += 2 * PI;

		qx = (m_trans_axis_inverse * m_rotate * m_trans_axis * QVector4D(qx, 1)).toVector3D();
		nx = (m_rotate * QVector4D(nx, 1)).toVector3D();
		wx = (m_rotate * QVector4D(wx, 1)).toVector3D();
		hx = (m_rotate * QVector4D(hx, 1)).toVector3D();

		qz = (m_trans_axis_inverse * m_rotate * m_trans_axis * QVector4D(qz, 1)).toVector3D();
		nz = (m_rotate * QVector4D(nz, 1)).toVector3D();
		wz = (m_rotate * QVector4D(wz, 1)).toVector3D();
		hz = (m_rotate * QVector4D(hz, 1)).toVector3D();
	}

	return 1;
}

/*
	// build rotation matrix
	float w_x, w_y, w_z, sa, ca;
	sa = sin(a);
	ca = cos(a);

	if (slice_type == 0) {
		w_x = -(nz.x());
		w_y = -(nz.y());
		w_z = -(nz.z());
	}
	else if (slice_type == 1) {
		w_x = nx.x();
		w_y = nx.y();
		w_z = nx.z();
	}
	else {
		w_x = ny.x();
		w_y = ny.y();
		w_z = ny.z();
	}

	float R_mat[9] = { w_x*w_x*(1-ca) + ca,			w_x*w_y*(1-ca) + w_z*sa,		w_x*w_z*(1-ca) - w_y*sa,
						w_y*w_x*(1-ca) - w_z*sa,		w_y*w_y*(1-ca) + ca,			w_y*w_z*(1-ca) + w_x*sa,
						w_z*w_x*(1-ca) + w_y*sa,		w_z*w_y*(1-ca) - w_x*sa,		w_z*w_z*(1-ca) + ca
	};

	// apply rotation matrix
	if (slice_type == 0) {
		qx = qx - P_axis;
		nx = apply_rotation_mat(R_mat, nx.x(), nx.y(), nx.z());
		wx = apply_rotation_mat(R_mat, wx.x(), wx.y(), wx.z());
		hx = apply_rotation_mat(R_mat, hx.x(), hx.y(), hx.z());
		qx = apply_rotation_mat(R_mat, qx.x(), qx.y(), qx.z());
		qx = qx + P_axis;

		qy = qy - P_axis;
		ny = apply_rotation_mat(R_mat, ny.x(), ny.y(), ny.z());
		wy = apply_rotation_mat(R_mat, wy.x(), wy.y(), wy.z());
		hy = apply_rotation_mat(R_mat, hy.x(), hy.y(), hy.z());
		qy = apply_rotation_mat(R_mat, qy.x(), qy.y(), qy.z());
		qy = qy + P_axis;

		r_z += a;
		if (r_z >= 2 * PI)
			r_z -= 2 * PI;
		else if (r_z < 0)
			r_z += 2 * PI;
	}
	else if (slice_type == 1) {
		qz = qz - P_axis;
		nz = apply_rotation_mat(R_mat, nz.x(), nz.y(), nz.z());
		wz = apply_rotation_mat(R_mat, wz.x(), wz.y(), wz.z());
		hz = apply_rotation_mat(R_mat, hz.x(), hz.y(), hz.z());
		qz = apply_rotation_mat(R_mat, qz.x(), qz.y(), qz.z());
		qz = qz + P_axis;

		qy = qy - P_axis;
		ny = apply_rotation_mat(R_mat, ny.x(), ny.y(), ny.z());
		wy = apply_rotation_mat(R_mat, wy.x(), wy.y(), wy.z());
		hy = apply_rotation_mat(R_mat, hy.x(), hy.y(), hy.z());
		qy = apply_rotation_mat(R_mat, qy.x(), qy.y(), qy.z());
		qy = qy + P_axis;

		r_x += a;
		if (r_x >= 2 * PI)
			r_x -= 2 * PI;
		else if (r_x < 0)
			r_x += 2 * PI;
	}
	else {
		qx = qx - P_axis;
		nx = apply_rotation_mat(R_mat, nx.x(), nx.y(), nx.z());
		wx = apply_rotation_mat(R_mat, wx.x(), wx.y(), wx.z());
		hx = apply_rotation_mat(R_mat, hx.x(), hx.y(), hx.z());
		qx = apply_rotation_mat(R_mat, qx.x(), qx.y(), qx.z());
		qx = qx + P_axis;

		qz = qz - P_axis;
		nz = apply_rotation_mat(R_mat, nz.x(), nz.y(), nz.z());
		wz = apply_rotation_mat(R_mat, wz.x(), wz.y(), wz.z());
		hz = apply_rotation_mat(R_mat, hz.x(), hz.y(), hz.z());
		qz = apply_rotation_mat(R_mat, qz.x(), qz.y(), qz.z());
		qz = qz + P_axis;

		r_y += a;
		if (r_y >= 2 * PI)
			r_y -= 2 * PI;
		else if (r_y < 0)
			r_y += 2 * PI;
	}

QVector3D DataCube::apply_rotation_mat(float R[], float x, float y, float z)
{
	return QVector3D(R[0] * x + R[1] * y + R[2] * z, R[3] * x + R[4] * y + R[5] * z, R[6] * x + R[7] * y + R[8] * z);
}
*/

int DataCube::move_center(int slice_type, float dx, float dy)
{
	QVector3D d, new_p;
	if (slice_type == 0) // z slice
		d = (dx * wz + dy * hz) * pixel_len_z;
	else if (slice_type == 1) // x slice
		d = (dx * wx + dy * hx) * pixel_len_x;
	else // y slice
		d = (dx * wy + dy * hy) * pixel_len_y;

	new_p = P_axis + d;
	if (new_p.x() < 0 || N_x < new_p.x() ||
		new_p.y() < 0 || N_y < new_p.y() ||
		new_p.z() < 0 || N_z * slice_thickness < new_p.z())
		return 0;

	P_axis = new_p;
	qz = P_axis - (wz * 7 / 4 + hz) * L_z / 2;
	qx = P_axis - (wx * 7 / 4 + hx) * L_x / 2;
	qy = P_axis - (wy * 7 / 4 + hy) * L_y / 2;

	return 1;
}

int DataCube::zoom_slice(int slice_type, float d)
{
	if (slice_type == 0) { // z slice
		if (L_z + d < N_max / 10 || 5 * N_max < L_z + d)
			return 0;

		L_z += d;
		pixel_len_z = L_z / (slice_pixel_num - 1);
		qz = qz - (wz * 7 / 4 + hz) * d / 2;
	}
	else if (slice_type == 1) { // x slice
		if (L_x + d < N_max / 10 || 5 * N_max < L_x + d)
			return 0;

		L_x += d;
		pixel_len_x = L_x / (slice_pixel_num - 1);
		qx = qx - (wx * 7 / 4 + hx) * d / 2;
	}
	else { // y slice
		if (L_y + d < N_max / 10 || 5 * N_max < L_y + d)
			return 0;

		L_y += d;
		pixel_len_y = L_y / (slice_pixel_num - 1);
		qy = qy - (wy * 7 / 4 + hy) * d / 2;
	}

	return 1;
}

int DataCube::slice_panning(int slice_type, float dx, float dy)
{
	// TODO: add limits?
	if (slice_type == 0) // z slice
		qz = qz + pixel_len_z * (dx * wz + dy * hz);
	else if (slice_type == 1) // x slice
		qx = qx + pixel_len_x * (dx * wx + dy * hx);
	else // y slice
		qy = qy + pixel_len_y * (dx * wy + dy * hy);

	return 1;
}

tuple<QVector3D, QVector3D, QVector3D, QVector3D, float, QVector3D, QVector3D, QVector3D, QVector3D, float, QVector3D, QVector3D, QVector3D, QVector3D, float> DataCube::get_slice_info()
{
	return { qz, nz, wz, hz, L_z, qx, nx, wx, hx, L_x, qy, ny, wy, hy, L_y };
}


/*
int* DataCube::get_DVR_screen()
{
	int* screen = (int*)malloc(dvr_pixel_num * dvr_pixel_num * sizeof(int));
	QVector3D center, start, temp;

	center = QVector3D(N_x / 2, N_y / 2, N_z / 2);
	start = (P_cam - center) * (d1 + d2) / (d1 + d2 + d3) + center - (ws + hs) * pixel_len_s * dvr_pixel_num / 2;
	start = start - (dvr_start_x * ws + dvr_start_y * hs) * pixel_len_s;

	for (int i = 0; i < dvr_pixel_num; i++) {
		temp = start;

		for (int j = 0; j < dvr_pixel_num; j++) {
			screen[dvr_pixel_num*i + j] = ray_casting(temp.x(), temp.y(), temp.z());
			temp = temp + ws * pixel_len_s;
		}
		start = start + hs * pixel_len_s;
	}

	return screen;
}

int DataCube::ray_casting(float x, float y, float z)
{
	int max_val = 0;
	float temp_len;
	QVector3D ray, cur;

	ray = QVector3D(x, y, z) - P_cam;
	temp_len = ray.length();
	ray = ray / temp_len;
	ray = ray * ray_len;
	cur = ray_penetrating_point(ray);

	while (1)
	{
		if (cur.x() < 0 || cur.y() < 0 || cur.z() < 0 ||
			cur.x() > N_x || cur.y() > N_y || cur.z() > N_z)
			break;

		int interpolated_value = trilinear_interpolation(-1, cur.x(), cur.y(), cur.z());
		if (max_val < interpolated_value)
			max_val = interpolated_value;

		cur = cur + ray;
	}

	return max_val;
}

QVector3D DataCube::ray_penetrating_point(QVector3D r)
{
	QVector3D shortest, temp;
	int found = 0;
	float a, b, c, x, y, z;
	a = r.x();
	b = r.y();
	c = r.z();

	if (a != 0) {
		x = 0;
		y = P_cam.y() + (x - P_cam.x()) * b / a;
		z = P_cam.z() + (x - P_cam.x()) * c / a;
		if (0 <= y && y <= N_y - 1 && 0 <= z && z <= N_z - 1) {
			temp = QVector3D(x, y, z) - P_cam;
			if (found == 0 || shortest.length() > temp.length()) {
				found = 1;
				shortest = temp;
			}
		}

		x = N_x - 1;
		y = P_cam.y() + (x - P_cam.x()) * b / a;
		z = P_cam.z() + (x - P_cam.x()) * c / a;
		if (0 <= y && y <= N_y - 1 && 0 <= z && z <= N_z - 1) {
			temp = QVector3D(x, y, z) - P_cam;
			if (found == 0 || shortest.length() > temp.length()) {
				found = 1;
				shortest = temp;
			}
		}
	}

	if (b != 0) {
		y = 0;
		x = P_cam.x() + (y - P_cam.y()) * a / b;
		z = P_cam.z() + (y - P_cam.y()) * c / b;
		if (0 <= x && x <= N_x - 1 && 0 <= z && z <= N_z - 1) {
			temp = QVector3D(x, y, z) - P_cam;
			if (found == 0 || shortest.length() > temp.length()) {
				found = 1;
				shortest = temp;
			}
		}

		y = N_y - 1;
		x = P_cam.x() + (y - P_cam.y()) * a / b;
		z = P_cam.z() + (y - P_cam.y()) * c / b;
		if (0 <= x && x <= N_x - 1 && 0 <= z && z <= N_z - 1) {
			temp = QVector3D(x, y, z) - P_cam;
			if (found == 0 || shortest.length() > temp.length()) {
				found = 1;
				shortest = temp;
			}
		}
	}

	if (c != 0) {
		z = 0;
		x = P_cam.x() + (z - P_cam.z()) * a / c;
		y = P_cam.y() + (z - P_cam.z()) * b / c;
		if (0 <= x && x <= N_x - 1 && 0 <= y && y <= N_y - 1) {
			temp = QVector3D(x, y, z) - P_cam;
			if (found == 0 || shortest.length() > temp.length()) {
				found = 1;
				shortest = temp;
			}
		}

		z = N_z - 1;
		x = P_cam.x() + (z - P_cam.z()) * a / c;
		y = P_cam.y() + (z - P_cam.z()) * b / c;
		if (0 <= x && x <= N_x - 1 && 0 <= y && y <= N_y - 1) {
			temp = QVector3D(x, y, z) - P_cam;
			if (found == 0 || shortest.length() > temp.length()) {
				found = 1;
				shortest = temp;
			}
		}
	}

	return shortest + P_cam;
}

int DataCube::rotate_DVR_screen(float dx, float dy)
{
	QMatrix4x4 m_rotate;
	m_rotate.setToIdentity();
	m_rotate.rotate(dx * 1800 / (PI * d1), -hs);
	m_rotate.rotate(dy * 1800 / (PI * d1), ws);

	QVector4D new_c, new_w, new_h, new_n;
	P_cam = (m_trans_center_inverse * m_rotate * m_trans_center * QVector4D(P_cam, 1)).toVector3D();
	ws = (m_rotate * QVector4D(ws, 1)).toVector3D();
	hs = (m_rotate * QVector4D(hs, 1)).toVector3D();
	ns = (m_rotate * QVector4D(ns, 1)).toVector3D();

	return 1;
}

int DataCube::panning_DVR_screen(float dx, float dy)
{
	// TODO: add limits?
	dvr_start_x += dx;
	dvr_start_y += dy;
	//qy = qy + pixel_len_y * (dx * wy + dy * hy);
	return 1;
}

int DataCube::zoom_DVR_screen(float d)
{
	if (L_z + d < N_max / 10 || 5 * N_max < L_z + d)
		return 0;

	L_s += d;
	pixel_len_s = L_s / (dvr_pixel_num - 1);
	//qz = qz - (wz + hz) * d / 2;

	return 1;
}

void DataCube::get_DVR_screen_GPU()
{
	int m_count = 0;
	QVector3D center, start, temp;
	QVector3D t1, t2, t3, t4;
	m_data.resize(dvr_pixel_num * dvr_pixel_num * 3 * 6);
	center = QVector3D(N_x / 2, N_y / 2, N_z / 2);
	start = (P_cam - center) * (d1 + d2) / (d1 + d2 + d3) + center - (ws + hs) * pixel_len_s * dvr_pixel_num / 2;
	start = start - (dvr_start_x * ws + dvr_start_y * hs) * pixel_len_s;

	for (int i = 0; i < dvr_pixel_num; i++) {
		temp = start;

		for (int j = 0; j < dvr_pixel_num; j++) {
			//screen[dvr_pixel_num*i + j] = ray_casting(temp.x(), temp.y(), temp.z());

			t1 = temp;
			t2 = temp + ws * pixel_len_s;
			t3 = temp + hs * pixel_len_s;
			t4 = t2 + hs * pixel_len_s;

			GLfloat *p = m_data.data() + m_count;
			//*p++ = temp.x();
			//*p++ = temp.y();
			//*p++ = temp.z();
			//*p++ = ns.x();
			//*p++ = ns.y();
			//*p++ = ns.z();
			//*p++ = 0.0;
			//*p++ = 1.0;
			//*p++ = 0.0;

			*p++ = t1.x();
			*p++ = t1.y();
			*p++ = t1.z();
			*p++ = t2.x();
			*p++ = t2.y();
			*p++ = t2.z();
			*p++ = t3.x();
			*p++ = t3.y();
			*p++ = t3.z();

			*p++ = t4.x();
			*p++ = t4.y();
			*p++ = t4.z();
			*p++ = t3.x();
			*p++ = t3.y();
			*p++ = t3.z();
			*p++ = t2.x();
			*p++ = t2.y();
			*p++ = t2.z();

			m_count += 3 * 6;

			temp = temp + ws * pixel_len_s;
		}
		start = start + hs * pixel_len_s;
	}
}
*/

