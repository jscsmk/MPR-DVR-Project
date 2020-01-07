#ifndef DATACUBE_H
#define DATACUBE_H

#include <tuple>
#include <QMatrix4x4>
#include <QVector>
#include <QVector4D>
#include <QVector3D>
#include <qmath.h>
#include <qopengl.h>

using namespace std;

class DataCube
{
public:
	DataCube();	
	void set_data(int *data, int x, int y, int z, int p, int a, int b, float t, int mi, int ma);
	void init_MPR();
	tuple<int, int, int, float> get_data_size();
	tuple<int, int, int, int, int> get_pixel_info();	
	int* get_raw_data();
	void get_slice(int slice_type, int *slice_data);
	tuple<int, int, int, int> get_coord(int slice_type, int m_x, int m_y);
	tuple<int, int, float> get_line_info(int slice_type);
	int move_slice(int from, int target, float distance);
	int rotate_slice(int slice_type, float a);
	int move_center(int slice_type, float dx, float dy);
	int zoom_slice(int slice_type, float d);
	int slice_panning(int slice_type, float dx, float dy);
	void toggle_border_line(int slice_type);
	tuple<QVector3D, QVector3D, QVector3D, QVector3D, float, QVector3D, QVector3D, QVector3D, QVector3D, float, QVector3D, QVector3D, QVector3D, QVector3D, float> get_slice_info();
	
	/*int* get_DVR_screen();
	int rotate_DVR_screen(float dx, float dy);	
	int panning_DVR_screen(float dx, float dy);
	int zoom_DVR_screen(float d);


	 TODO:
		add methods for MPR
		 - thickening the slice -> get avg/max/min
		 - show border -> color?
		add methods for DVR(ray-casting)
		 - rotate
		 - panning
		 - zooming
		 - windowing
		 - show border
	*/


protected:
	int trilinear_interpolation(int slice_type, float x, float y, float z);
	tuple<int, int, int> closest_neighbor(float x, float y, float z);
	//int ray_casting(float x, float y, float z);
	//QVector3D ray_penetrating_point(QVector3D r);

private:	
	int *data_3d;							// 3d data points
	//int *slice;
	int N_x, N_y, N_z, N_max,				// size of total data	
		slice_pixel_num,					// number of pixels of slice
		rescale_slope, rescale_intercept,	// dicom pixel rescale values
		x_border_visible,
		y_border_visible,
		z_border_visible,					// 0: border invisible, 1: border visible
		pixel_min, pixel_max;
	float L_x, L_y, L_z,					// size of each slice
		  r_z, r_x, r_y,					// angle(rad, 0~2pi) of rotation on each slice	
		  pixel_len_x,							
		  pixel_len_y,
		  pixel_len_z,						// length of pixel vectors
  		  slice_thickness;
	QVector3D P_axis,						// position of axis intersection			  
			  nx, ny, nz,					// normal vectors of slices & scrren
			  wx, wy, wz,					// horizontal pixel vectors of slices & screen
			  hx, hy, hz,					// vertical pixel vectors of slices & screen
			  qx, qy, qz;					// position of top-left point of slices
	QMatrix4x4 m_trans_center, m_trans_center_inverse; // trans matrix to move center of data to origin point 	
};
#endif