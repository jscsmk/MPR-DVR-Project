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

void DataCube::set_data(int *data, int *mask, int n_m, int x, int y, int z, int p, int a, int b, float t, int mi, int ma)
{
	data_3d = data;
	mask_3d = mask;
	N_mask = n_m;
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

	wx = QVector3D(0, -1, 0);
	hx = QVector3D(0, 0, 1);
	wy = QVector3D(1, 0, 0);
	hy = QVector3D(0, 0, 1);
	wz = QVector3D(1, 0, 0);
	hz = QVector3D(0, 1, 0);

	qz = P_axis - QVector3D(L_z * 7 / 8, L_z / 2, 0);
	qx = P_axis + QVector3D(0, L_x * 7 / 8, -L_x / 2);
	qy = P_axis - QVector3D(L_y * 7 / 8, 0, L_y / 2);

	r_x = PI / 2;
	r_y = 0;
	r_z = 0;
}

tuple<int, int, int, float> DataCube::get_data_size()
{
	return { N_x, N_y, N_z, slice_thickness };
}
tuple<int, int, int, int, int, int> DataCube::get_pixel_info()
{
	return {slice_pixel_num, rescale_slope, rescale_intercept, pixel_min, pixel_max, N_mask };
}
int* DataCube::get_raw_data()
{
	return data_3d;
}
int* DataCube::get_cur_mask()
{
	return mask_3d;
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

void DataCube::get_slice(int slice_type, int *slice_data, int *mask_data)
{
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
			int interpolated_data, cn_x, cn_y, cn_z;
			interpolated_data = trilinear_interpolation(slice_type, temp.x(), temp.y(), temp.z());
			tie(cn_x, cn_y, cn_z) = closest_neighbor(temp.x(), temp.y(), temp.z());
			slice_data[slice_pixel_num*i * 7 / 4 + j] = interpolated_data;

			for (int m = 0; m < N_mask; m++) {
				mask_data[N_mask*(slice_pixel_num*i * 7 / 4 + j) + m] = cn_x > 0 ? mask_3d[N_mask*(N_x*N_y*cn_z + N_x * cn_y + cn_x) + m] : 0;
			}
			temp = temp + w;
		}
		start = start + h;
	}
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
	else {
		draw_border = y_border_visible;
		pl = pixel_len_y;
	}

	if (x < -2*pl || y < -2*pl || z < -2*pl / slice_thickness || x >= N_x - 1 + 2*pl || y >= N_y - 1 + 2*pl || z >= N_z - 1 + 2*pl/slice_thickness ) {
		// if out of range, return min val
		return pixel_min;
	}

	if (x < 0 || y < 0 || z < 0 || x >= N_x - 1 || y >= N_y - 1 || z >= N_z - 1) {
		// if border, return -1
		if (draw_border == 1)
			return pixel_min - 1;

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
	z /= slice_thickness;
	if (x < 0 || y < 0 || z < 0 || x >= N_x - 1 || y >= N_y - 1 || z >= N_z - 1)
		return { -1, -1, -1 };

	return { floor(x + 0.5), floor(y + 0.5), floor(z + 0.5) };
}

tuple<float, float, float, int> DataCube::get_coord(int slice_type, int m_x, int m_y)
{
	QVector3D point;
	int pixel_v;

	if (slice_type == 0) // z slice
		point = qz + pixel_len_z * (m_x * wz + m_y * hz);
	else if (slice_type == 1) // x slice
		point = qx + pixel_len_x * (m_x * wx + m_y * hx);
	else // y slice
		point = qy + pixel_len_y * (m_x * wy + m_y * hy);

	pixel_v = trilinear_interpolation(slice_type, point.x(), point.y(), point.z());
	return { point.x(), point.y(), point.z(), pixel_v};
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
