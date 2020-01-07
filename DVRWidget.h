#ifndef DVRWIDGET_H
#define DVRWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include "data_cube.h"
#include <tuple>
using namespace std;

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class DVRWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT

public:
	DVRWidget(DataCube *d, float r, int a, int b, int p, int s);
	~DVRWidget();
	void set_data(DataCube *d, float r, int a, int b);
	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

public slots:
	void init_all();
	void init_geometry();
	void init_windowing();	
	void cleanup();
	void toggle_mode();
	void toggle_border_line();
	void toggle_axial_slice();	
	void toggle_sagittal_slice();	
	void toggle_coronal_slice();	
	void get_slice_info();

signals:
	void windowing_info_sig(QString msg);
	void coord_info_sig(QString msg);

protected:
	void _init_geometry();
	void _init_windowing();
	void set_tex_samplers();
	void set_slice_texture();
	void setupAttribs(QOpenGLBuffer *b);
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;	
	QVector3D get_arcball_pos(int m_x, int m_y);

private:	
	//QOpenGLContext *m_context;
	GLuint m_target_texture, m_depthbuffer, m_framebuffer;
	DataCube *data_cube;
	QOpenGLShaderProgram *m_program, *m_program_2;
	QOpenGLVertexArrayObject m_vao, m_vao_2;
	QOpenGLBuffer m_vbo, m_vbo_2;
	QVector3D P_screen, ns, ws, hs;
	QVector3D qz, nz, wz, hz, qx, nx, wx, hx, qy, ny, wy, hy;
	QVector<GLfloat> m_data, m_data_2;
	short *tex_3d_data, *tex_3d_border, *tex_slice_depth;
	int N_x, N_y, N_z, N_max, L_z, L_x, L_y,
		screen_size,
		dvr_pixel_num,
		rescale_slope, rescale_intercept,	// dicom pixel rescale values
		mouse_last_x, mouse_last_y;		
	bool is_first,
		 mode,	// flase: OTF, true: MIP		
		 border_line_visible,
		 axial_slice_visible,
		 sagittal_slice_visible,
		 coronal_slice_visible;
	QVector3D arcball_last_pos;
	float L_s,
		  slice_thickness,
		  pixel_len_s,
		  unit_ray_len,
		  screen_dist,
		  window_level, window_width,
		  cur_center_x, cur_center_y;
	int m_projMatrixLoc, m_mvMatrixLoc,
		volume_data_loc, x_loc, y_loc, z_loc, slice_thickness_loc,
		P_screen_loc, v_width_loc, v_height_loc, v_normal_loc,
		window_level_loc, window_width_loc,
		mode_loc, border_visible_loc,
		size_test_loc,
		axial_visible_loc, sagittal_visible_loc, coronal_visible_loc;

	int m_projMatrixLoc_2, m_mvMatrixLoc_2,
		P_screen_loc_2,
		ns_loc,
		ws_loc,
		hs_loc,
		qx_loc,
		nx_loc,
		wx_loc,
		hx_loc,
		qy_loc,
		ny_loc,
		wy_loc,
		hy_loc,
		qz_loc,
		nz_loc,
		wz_loc,
		hz_loc,
		L_x_loc,
		L_y_loc,
		L_z_loc;

	QOpenGLTexture *m_texture, *m_texture_border, *m_texture_slice;
	QMatrix4x4 m_proj, m_camera, m_trans_center, m_trans_center_inverse;
};
#endif