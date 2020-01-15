#include "DVRWidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QCoreApplication>
#include <math.h>
#include <sstream>
#include <iostream>
#include <string.h>
using namespace std;
#define PI 3.141592


DVRWidget::DVRWidget(DataCube *d, float r, int a, int b, int p, int s)
{
	dvr_pixel_num = p;
	screen_size = s;

	m_data.resize(dvr_pixel_num * dvr_pixel_num * 3 * 6 * 7 / 4);
	int m_count = 0;
	for (int j = 0; j < dvr_pixel_num; j++) {
		for (int i = 0; i < dvr_pixel_num * 7 / 4; i++) {
			GLfloat *p = m_data.data() + m_count;

			*p++ = i;
			*p++ = j;
			*p++ = 0;
			*p++ = i + 1;
			*p++ = j;
			*p++ = 0;
			*p++ = i;
			*p++ = j + 1;
			*p++ = 0;

			*p++ = i + 1;
			*p++ = j;
			*p++ = 0;
			*p++ = i + 1;
			*p++ = j + 1;
			*p++ = 0;
			*p++ = i;
			*p++ = j + 1;
			*p++ = 0;

			m_count += 18;
		}
	}

	m_data_2.resize(3 * dvr_pixel_num * dvr_pixel_num * 3 * 6 * 7 / 4);
	m_count = 0;
	for (int j = 0; j < 3 * dvr_pixel_num; j++) {
		for (int i = 0; i < dvr_pixel_num * 7 / 4; i++) {
			GLfloat *p = m_data_2.data() + m_count;

			*p++ = i;
			*p++ = j;
			*p++ = 0;
			*p++ = i + 1;
			*p++ = j;
			*p++ = 0;
			*p++ = i;
			*p++ = j + 1;
			*p++ = 0;

			*p++ = i + 1;
			*p++ = j;
			*p++ = 0;
			*p++ = i + 1;
			*p++ = j + 1;
			*p++ = 0;
			*p++ = i;
			*p++ = j + 1;
			*p++ = 0;

			m_count += 18;
		}
	}

	is_first = true;

	tex_3d_data = NULL;
	tex_3d_border = NULL;
	tex_slice_depth = NULL;
	tex_octree_smin = NULL;
	tex_octree_smax = NULL;

	m_program = new QOpenGLShaderProgram;
	m_program_2 = new QOpenGLShaderProgram;
	m_texture = new QOpenGLTexture(QOpenGLTexture::Target3D);
	m_texture_border = new QOpenGLTexture(QOpenGLTexture::Target3D);
	m_texture_slice = new QOpenGLTexture(QOpenGLTexture::Target2D);
	m_texture_octree_smin = new QOpenGLTexture(QOpenGLTexture::Target1D);
	m_texture_octree_smax = new QOpenGLTexture(QOpenGLTexture::Target1D);

	tex_slice_depth = new short[3 * dvr_pixel_num * dvr_pixel_num * 7 / 4];

	octree_depth = 4; // min = 0
	octree_length = (int)((pow(8, octree_depth + 1) - 1) / 7); // size: (1 + 8 + 8^2 + ... + 8^n)

	set_data(d, r, a, b);
}

DVRWidget::~DVRWidget()
{
	cleanup();
}

tuple<short, short> DVRWidget::set_smin_smax(int depth, int idx, float x0, float x1, float y0, float y1, float z0, float z1)
{
	short smin, smax;
	smin = 32767;
	smax = 0;

	if (depth == octree_depth) { // leaf node
		int x_start, x_end, y_start, y_end, z_start, z_end;
		x_start = floor(x0);
		y_start = floor(y0);
		z_start = floor(z0);
		x_end = min(N_x-1, (int)ceil(x1));
		y_end = min(N_y-1, (int)ceil(y1));
		z_end = min(N_z-1, (int)ceil(z1));

		for (int k = z_start; k <= z_end; k++) {
			for (int j = y_start; j <= y_end; j++) {
				for (int i = x_start; i <= x_end; i++) {
					smin = min(smin, tex_3d_data[N_x * N_y * k + N_x * j + i]);
					smax = max(smax, tex_3d_data[N_x * N_y * k + N_x * j + i]);
				}
			}
		}
	}
	else { // not leaf node
		int first_child_idx = 8 * idx + 1;
		float xm, ym, zm;
		short temp_min, temp_max;
		xm = (x0 + x1) / 2;
		ym = (y0 + y1) / 2;
		zm = (z0 + z1) / 2;

		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx, x0, xm, y0, ym, z0, zm);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx+1, xm, x1, y0, ym, z0, zm);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx+2, x0, xm, ym, y1, z0, zm);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx+3, xm, x1, ym, y1, z0, zm);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx+4, x0, xm, y0, ym, zm, z1);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx+5, xm, x1, y0, ym, zm, z1);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx+6, x0, xm, ym, y1, zm, z1);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
		tie(temp_min, temp_max) = set_smin_smax(depth + 1, first_child_idx+7, xm, x1, ym, y1, zm, z1);
		smin = min(smin, temp_min);
		smax = max(smax, temp_max);
	}

	tex_octree_smin[idx] = smin;
	tex_octree_smax[idx] = smax;
	return {smin, smax};
}

void DVRWidget::set_data(DataCube *d, float r, int a, int b)
{
	mode = false;
	skipping = true;
	border_line_visible = false;
	axial_slice_visible = false;
	sagittal_slice_visible = false;
	coronal_slice_visible = false;

	data_cube = d;
	unit_ray_len = r;
	rescale_slope = a;
	rescale_intercept = b;

	tie(N_x, N_y, N_z, slice_thickness) = data_cube->get_data_size();
	N_max = max(N_x, max(N_y, (int)(N_z * slice_thickness)));
	screen_dist = N_max * 5;

	int *raw = data_cube->get_raw_data();

	free(tex_3d_data);
	free(tex_3d_border);
	free(tex_octree_smin);
	free(tex_octree_smax);

	// build tex data of 3d volume data
	tex_3d_data = new short[N_x * N_y * N_z];
	for (int i = 0; i < N_x * N_y * N_z; i++) {
		int temp = raw[i];
		temp = a * temp + b; // rescale stored valued to HU (-1000~3096)
		temp += 1000;		 // move to 0~4095
		temp *= 8;			 // scale to 0~32768
		if (temp < 0)
			temp = 0;
		else if (temp > 32767)
			temp = 32767;

		tex_3d_data[i] = temp;
	}

	// build tex data of border data
	int t = 3;
	tex_3d_border = new short[N_x * N_y * N_z];
	for (int k = 0; k < N_z; k++) {
		for (int j = 0; j < N_y; j++) {
			for (int i = 0; i < N_x; i++) {
				int temp = 32767;
				if (t < i && i < N_x - t - 1 && t < j && j < N_y - t - 1)
					temp = 0;
				if (t < k && k < N_z - t - 1 && t < j && j < N_y - t - 1)
					temp = 0;
				if (t < i && i < N_x - t - 1 && t < k && k < N_z - t - 1)
					temp = 0;

				tex_3d_border[N_x * N_y * k + N_x * j + i] = temp;
			}
		}
	}

	// build tex data of octree
	tex_octree_smin = new short[octree_length];
	tex_octree_smax = new short[octree_length];
	set_smin_smax(0, 0, 0, N_x, 0, N_y, 0, N_z);

	m_trans_center.setToIdentity();
	m_trans_center.translate(QVector3D(-N_x / 2, -N_y / 2, -N_z * slice_thickness / 2));
	m_trans_center_inverse.setToIdentity();
	m_trans_center_inverse.translate(QVector3D(N_x / 2, N_y / 2, N_z * slice_thickness / 2));

	tie(qz, nz, wz, hz, L_z, qx, nx, wx, hx, L_x, qy, ny, wy, hy, L_y) = data_cube->get_slice_info();

	_init_windowing();
	_init_geometry();

	if (!is_first) {
		set_tex_samplers();
		update();
	}
}

QSize DVRWidget::minimumSizeHint() const
{
	return QSize(50, 50);
}

QSize DVRWidget::sizeHint() const
{
	return QSize(screen_size * 7 / 4, screen_size);
}

void DVRWidget::cleanup()
{
	if (m_program == nullptr && m_program_2 == nullptr)
		return;

	makeCurrent();

	if (m_program != nullptr) {
		m_vbo.destroy();
		delete m_program;
		m_program = 0;
	}
	if (m_program_2 != nullptr) {
		m_vbo_2.destroy();
		delete m_program_2;
		m_program_2 = 0;
	}

	doneCurrent();
}

void DVRWidget::toggle_mode()
{
	mode = mode ? false : true;
	update();
}
void DVRWidget::toggle_skipping()
{
	skipping = skipping ? false : true;
	update();
}
void DVRWidget::toggle_border_line()
{
	border_line_visible = border_line_visible ? false : true;
	update();
}
void DVRWidget::toggle_axial_slice()
{
	axial_slice_visible = axial_slice_visible ? false : true;
	update();
}
void DVRWidget::toggle_sagittal_slice()
{
	sagittal_slice_visible = sagittal_slice_visible ? false : true;
	update();
}
void DVRWidget::toggle_coronal_slice()
{
	coronal_slice_visible = coronal_slice_visible ? false : true;
	update();
}

void DVRWidget::init_all()
{
	_init_windowing();
	_init_geometry();
	update();
}
void DVRWidget::init_geometry()
{
	_init_geometry();
	update();
}
void DVRWidget::init_windowing()
{
	_init_windowing();
	update();
}

void DVRWidget::_init_geometry()
{
	L_s = N_max;
	pixel_len_s = L_s / (dvr_pixel_num - 1);
	cur_center_x = screen_size * 7 / 8;
	cur_center_y = screen_size / 2;

	P_screen = QVector3D(N_x / 2, N_y / 2, N_z * slice_thickness / 2) + QVector3D(L_s * 7 / 8, -screen_dist, L_s / 2);
	ws = QVector3D(-1, 0, 0);
	hs = QVector3D(0, 0, -1);
	ns = QVector3D(0, 1, 0);

	arcball_last_pos = get_arcball_pos(10, 10);
	QVector3D arcball_pos = get_arcball_pos(20, 20);
	do_rotate(arcball_pos);
}

void DVRWidget::_init_windowing()
{
	window_level = 200;
	window_width = 200;
}

void DVRWidget::get_slice_info()
{
	tie(qz, nz, wz, hz, L_z, qx, nx, wx, hx, L_x, qy, ny, wy, hy, L_y) = data_cube->get_slice_info();
	update();
}

void DVRWidget::initializeGL()
{
	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &DVRWidget::cleanup);
	initializeOpenGLFunctions();
	glClearColor(0, 0, 0, 1);

	// set shader program
	m_program->removeAllShaders();
	m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/ray_casting.vert");
	m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/ray_casting.frag");
	m_program->bindAttributeLocation("vert_in", 0);
	m_program->link();
	m_program->bind();

	m_projMatrixLoc = m_program->uniformLocation("projMatrix");
	m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");

	volume_data_loc = m_program->uniformLocation("volume_data");
	x_loc = m_program->uniformLocation("N_x");
	y_loc = m_program->uniformLocation("N_y");
	z_loc = m_program->uniformLocation("N_z");
	slice_thickness_loc = m_program->uniformLocation("slice_thickness");

	window_level_loc = m_program->uniformLocation("window_level");
	window_width_loc = m_program->uniformLocation("window_width");
	mode_loc = m_program->uniformLocation("mode");
	border_visible_loc = m_program->uniformLocation("border_visible");
	axial_visible_loc = m_program->uniformLocation("axial_visible");
	sagittal_visible_loc = m_program->uniformLocation("sagittal_visible");
	coronal_visible_loc = m_program->uniformLocation("coronal_visible");

	P_screen_loc = m_program->uniformLocation("P_screen");
	v_width_loc = m_program->uniformLocation("v_width");
	v_height_loc = m_program->uniformLocation("v_height");
	v_normal_loc = m_program->uniformLocation("v_normal");

	set_tex_samplers();

	// Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
	// implementations this is optional and support may not be present
	// at all. Nonetheless the below code works in all cases and makes
	// sure there is a VAO when one is needed.
	//m_vao.destroy();
	m_vao.create();
	QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

	// Setup our vertex buffer object.
	m_vbo.create();
	m_vbo.bind();
	m_vbo.allocate(m_data.constData(), 3 * 6 * dvr_pixel_num * dvr_pixel_num * 7 / 4 * sizeof(GLfloat));

	// Store the vertex attribute bindings for the program.
	setupAttribs(&m_vbo);

	// Our camera never changes in this example.
	m_camera.setToIdentity();
	m_camera.translate(-dvr_pixel_num * 7 / 8, -dvr_pixel_num/2, -dvr_pixel_num * 1.2);

	m_program->release();

	// set shader for slices
	m_program_2->removeAllShaders();
	m_program_2->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/slice_depth.vert");
	m_program_2->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/slice_depth.frag");
	m_program_2->bindAttributeLocation("vert_in", 0);
	m_program_2->link();
	m_program_2->bind();

	m_projMatrixLoc_2 = m_program_2->uniformLocation("projMatrix");
	m_mvMatrixLoc_2 = m_program_2->uniformLocation("mvMatrix");
	P_screen_loc_2 = m_program_2->uniformLocation("P_screen");
	ns_loc = m_program_2->uniformLocation("ns");
	ws_loc = m_program_2->uniformLocation("ws");
	hs_loc = m_program_2->uniformLocation("hs");
	qx_loc = m_program_2->uniformLocation("qx");
	nx_loc = m_program_2->uniformLocation("nx");
	wx_loc = m_program_2->uniformLocation("wx");
	hx_loc = m_program_2->uniformLocation("hx");
	qy_loc = m_program_2->uniformLocation("qy");
	ny_loc = m_program_2->uniformLocation("ny");
	wy_loc = m_program_2->uniformLocation("wy");
	hy_loc = m_program_2->uniformLocation("hy");
	qz_loc = m_program_2->uniformLocation("qz");
	nz_loc = m_program_2->uniformLocation("nz");
	wz_loc = m_program_2->uniformLocation("wz");
	hz_loc = m_program_2->uniformLocation("hz");
	L_x_loc = m_program_2->uniformLocation("L_x");
	L_y_loc = m_program_2->uniformLocation("L_y");
	L_z_loc = m_program_2->uniformLocation("L_z");

	m_vao_2.create();
	QOpenGLVertexArrayObject::Binder vaoBinder_2(&m_vao_2);

	// Setup our vertex buffer object.
	m_vbo_2.create();
	m_vbo_2.bind();
	m_vbo_2.allocate(m_data.constData(), 3 * 6 * dvr_pixel_num * dvr_pixel_num * 7 / 4 * sizeof(GLfloat));

	// Store the vertex attribute bindings for the program.
	setupAttribs(&m_vbo_2);

	set_slice_texture();

	m_program_2->release();

	is_first = false;
}

void DVRWidget::setupAttribs(QOpenGLBuffer *b)
{
	b->bind();
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	b->release();
}

void DVRWidget::set_tex_samplers()
{
	// set 3d data sampler
	m_texture->destroy();
	m_texture->create();
	m_texture->setSize(N_x, N_y, N_z);
	m_texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
	m_texture->setFormat(QOpenGLTexture::R16_UNorm);
	m_texture->setBorderColor(QColor(0, 0, 0));
	m_texture->setWrapMode(QOpenGLTexture::ClampToBorder);
	m_texture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Int16);
	m_texture->setData(QOpenGLTexture::Red, QOpenGLTexture::Int16, tex_3d_data);

	// set border sampler
	m_texture_border->destroy();
	m_texture_border->create();
	m_texture_border->setSize(N_x, N_y, N_z);
	m_texture_border->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
	m_texture_border->setFormat(QOpenGLTexture::R16_UNorm);
	m_texture_border->setBorderColor(QColor(0, 0, 0));
	m_texture_border->setWrapMode(QOpenGLTexture::ClampToBorder);
	m_texture_border->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Int16);
	m_texture_border->setData(QOpenGLTexture::Red, QOpenGLTexture::Int16, tex_3d_border);

	// set octree sampler
	m_texture_octree_smin->destroy();
	m_texture_octree_smin->create();
	m_texture_octree_smin->setSize(octree_length);
	m_texture_octree_smin->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
	m_texture_octree_smin->setFormat(QOpenGLTexture::R16_UNorm);
	m_texture_octree_smin->setWrapMode(QOpenGLTexture::ClampToEdge);
	m_texture_octree_smin->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Int16);
	m_texture_octree_smin->setData(QOpenGLTexture::Red, QOpenGLTexture::Int16, tex_octree_smin);
	m_texture_octree_smax->destroy();
	m_texture_octree_smax->create();
	m_texture_octree_smax->setSize(octree_length);
	m_texture_octree_smax->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
	m_texture_octree_smax->setFormat(QOpenGLTexture::R16_UNorm);
	m_texture_octree_smax->setWrapMode(QOpenGLTexture::ClampToEdge);
	m_texture_octree_smax->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::Int16);
	m_texture_octree_smax->setData(QOpenGLTexture::Red, QOpenGLTexture::Int16, tex_octree_smax);
}

void DVRWidget::set_slice_texture()
{
	glGenFramebuffers(1, &m_framebuffer);
	glEnable(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	//glDeleteTextures(1, &m_target_texture);
	glGenTextures(1, &m_target_texture);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_target_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen_size * 7 / 4, screen_size, 0, GL_RGB, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_size * 7 / 4, screen_size, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0); // release

	//glDeleteRenderbuffers(1, &m_depthbuffer);
	glGenRenderbuffers(1, &m_depthbuffer);
	glEnable(GL_RENDERBUFFER);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screen_size * 7 / 4, screen_size);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_target_texture, 0);

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // release
}

void DVRWidget::paintGL()
{
	// build slice depth texture(CPU)
	/*
	float depth, pr;
	QVector3D vec_from_q, pr_w, pr_h;
	QVector3D temp_row = P_screen;

	for (int j = 0; j < dvr_pixel_num; j++) {
		QVector3D temp_col = temp_row;
		for (int i = 0; i < dvr_pixel_num * 7 / 4; i++) {
			int idx = j * dvr_pixel_num * 7 / 4 + i;

			depth = (QVector3D::dotProduct(qx, nx) - QVector3D::dotProduct(temp_col, nx)) / QVector3D::dotProduct(ns, nx);
			vec_from_q = temp_col + depth * ns - qx;
			pr = QVector3D::dotProduct(wx, vec_from_q);
			pr_w = wx * pr;
			pr_h = vec_from_q - pr_w;

			// check range
			if (QVector3D::dotProduct(pr_w, wx) < 0 || QVector3D::dotProduct(pr_h, hx) < 0 || pr_w.length() > L_x * 7 / 4 || pr_h.length() > L_x)
				tex_slice_depth[3 * idx] = 0;
			else
				tex_slice_depth[3 * idx] = (short)depth;

			depth = (QVector3D::dotProduct(qy, ny) - QVector3D::dotProduct(temp_col, ny)) / QVector3D::dotProduct(ns, ny);
			vec_from_q = temp_col + depth * ns - qy;
			pr = QVector3D::dotProduct(wy, vec_from_q);
			pr_w = wy * pr;
			pr_h = vec_from_q - pr_w;

			// check range
			if (QVector3D::dotProduct(pr_w, wy) < 0 || QVector3D::dotProduct(pr_h, hy) < 0 || pr_w.length() > L_y * 7 / 4 || pr_h.length() > L_y)
				tex_slice_depth[3 * idx + 1] = 0;
			else
				tex_slice_depth[3 * idx + 1] = (short)depth;

			depth = (QVector3D::dotProduct(qz, nz) - QVector3D::dotProduct(temp_col, nz)) / QVector3D::dotProduct(ns, nz);
			vec_from_q = temp_col + depth * ns - qz;
			pr = QVector3D::dotProduct(wz, vec_from_q);
			pr_w = wz * pr;
			pr_h = vec_from_q - pr_w;

			// check range
			if (QVector3D::dotProduct(pr_w, wz) < 0 || QVector3D::dotProduct(pr_h, hz) < 0 || pr_w.length() > L_z * 7 / 4 || pr_h.length() > L_z)
				tex_slice_depth[3 * idx + 2] = 0;
			else
				tex_slice_depth[3 * idx + 2] = (short)depth;

			temp_col += ws * pixel_len_s;
		}
		temp_row += hs * pixel_len_s;
	}

	//m_texture_slice->setData(QOpenGLTexture::RGB, QOpenGLTexture::Int16, tex_slice_depth);
	*/


	// start painting
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


	// first pass
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	QOpenGLVertexArrayObject::Binder vaoBinder_2(&m_vao_2);
	m_program_2->bind();
	m_program_2->setUniformValue(m_mvMatrixLoc_2, m_camera);
	m_program_2->setUniformValue(m_projMatrixLoc_2, m_proj);

	m_program_2->setUniformValue(P_screen_loc_2, P_screen);
	m_program_2->setUniformValue(ns_loc, ns);
	m_program_2->setUniformValue(ws_loc, ws * pixel_len_s);
	m_program_2->setUniformValue(hs_loc, hs * pixel_len_s);

	m_program_2->setUniformValue(qx_loc, qx);
	m_program_2->setUniformValue(nx_loc, nx);
	m_program_2->setUniformValue(wx_loc, wx);
	m_program_2->setUniformValue(hx_loc, hx);

	m_program_2->setUniformValue(qy_loc, qy);
	m_program_2->setUniformValue(ny_loc, ny);
	m_program_2->setUniformValue(wy_loc, wy);
	m_program_2->setUniformValue(hy_loc, hy);

	m_program_2->setUniformValue(qz_loc, qz);
	m_program_2->setUniformValue(nz_loc, nz);
	m_program_2->setUniformValue(wz_loc, wz);
	m_program_2->setUniformValue(hz_loc, hz);

	m_program_2->setUniformValue(L_x_loc, L_x);
	m_program_2->setUniformValue(L_y_loc, L_y);
	m_program_2->setUniformValue(L_z_loc, L_z);

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_FRAMEBUFFER);
	glViewport(0, 0, screen_size * 7 / 4, screen_size);
	glDrawArrays(GL_TRIANGLES, 0, 6 * dvr_pixel_num * dvr_pixel_num * 7 / 4);
	m_program_2->release();


	// second pass
	glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
	QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
	m_program->bind();

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_target_texture);
	m_program->setUniformValue("slice_data", GL_TEXTURE0);

	m_program->setUniformValue(m_mvMatrixLoc, m_camera);
	m_program->setUniformValue(x_loc, N_x);
	m_program->setUniformValue(y_loc, N_y);
	m_program->setUniformValue(z_loc, N_z);
	m_program->setUniformValue(slice_thickness_loc, slice_thickness);

	m_program->setUniformValue(m_projMatrixLoc, m_proj);
	m_program->setUniformValue(P_screen_loc, P_screen);
	m_program->setUniformValue(v_width_loc, ws * pixel_len_s);
	m_program->setUniformValue(v_height_loc, hs * pixel_len_s);
	m_program->setUniformValue(v_normal_loc, ns * unit_ray_len);
	m_program->setUniformValue(window_level_loc, (window_level + 1000) / 4096);
	m_program->setUniformValue(window_width_loc, window_width / 4096);
	m_program->setUniformValue(mode_loc, mode);
	m_program->setUniformValue(m_program->uniformLocation("skipping"), skipping);
	m_program->setUniformValue(border_visible_loc, border_line_visible);
	m_program->setUniformValue(axial_visible_loc, axial_slice_visible);
	m_program->setUniformValue(sagittal_visible_loc, sagittal_slice_visible);
	m_program->setUniformValue(coronal_visible_loc, coronal_slice_visible);
	m_program->setUniformValue(m_program->uniformLocation("octree_length"), octree_length);
	m_program->setUniformValue(m_program->uniformLocation("octree_max_depth"), octree_depth);
	m_texture->bind(2);
	//m_texture_border->bind(3);
	//m_texture_octree_smin->bind(3);
	m_texture_octree_smax->bind(3);
	m_program->setUniformValue("volume_data", 2);
	//m_program->setUniformValue("border_data", 3);
	//m_program->setUniformValue("octree_smin_data", 3);
	m_program->setUniformValue("octree_smax_data", 3);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, screen_size * 7 / 4, screen_size);
	glDrawArrays(GL_TRIANGLES, 0, 6 * dvr_pixel_num * dvr_pixel_num * 7 / 4);

	m_texture->release();
	m_texture_border->release();
	//m_texture_octree_smin->release();
	m_texture_octree_smax->release();
	m_program->release();
	glBindTexture(GL_TEXTURE_2D, 0);


	// emit windowing info
	QString msg = "WL: " + QString::number(floor(window_level)) + "\nWW: " + QString::number(floor(window_width));
	emit windowing_info_sig(msg);
}

void DVRWidget::resizeGL(int w, int h)
{
	m_proj.setToIdentity();
	m_proj.perspective(45.0f, GLfloat(w) / h, 0.01f, 10000.0f);
}

void DVRWidget::mousePressEvent(QMouseEvent *event)
{
	mouse_last_x = event->x();
	mouse_last_y = event->y();
	arcball_last_pos = get_arcball_pos(event->x(), event->y());
}

void DVRWidget::do_rotate(QVector3D arcball_pos) {
	QVector3D arcball_d = arcball_pos - arcball_last_pos;
	float d = arcball_d.length();
	float arcball_radius = 500.0 * screen_size / L_s;
	float a = acos(1 - d * d / (2 * arcball_radius * arcball_radius));
	QVector3D cross = QVector3D::crossProduct(arcball_last_pos, arcball_pos);
	QVector3D rotate_axis = cross.x() * ws - cross.y() * hs - cross.z() * ns;

	QMatrix4x4 m_rotate;
	m_rotate.setToIdentity();
	m_rotate.rotate(a * 2 * 180 / PI, rotate_axis);

	P_screen = (m_trans_center_inverse * m_rotate * m_trans_center * QVector4D(P_screen, 1)).toVector3D();
	ws = (m_rotate * QVector4D(ws, 1)).toVector3D();
	hs = (m_rotate * QVector4D(hs, 1)).toVector3D();
	ns = (m_rotate * QVector4D(ns, 1)).toVector3D();
}

void DVRWidget::mouseMoveEvent(QMouseEvent *event) // mouse click event
{
	if (event->buttons() & Qt::LeftButton) { // when left clicked = rotation
		QVector3D arcball_pos = get_arcball_pos(event->x(), event->y());
		do_rotate(arcball_pos);
		mouse_last_x = event->x();
		mouse_last_y = event->y();
		arcball_last_pos = arcball_pos;
		update();
	}
	else if (event->buttons() & Qt::RightButton) { // when Right clicked = windowing
		float dx = (float)(event->x() - mouse_last_x);
		float dy = (float)(mouse_last_y - event->y());
		window_width += dx;
		window_level += dy;

		if (window_width < 10)
			window_width = 10;
		if (window_width > 4000)
			window_width = 4000;
		if (window_level < -1000)
			window_level = -1000;
		if (window_level > 3096)
			window_level = 3096;

		mouse_last_x = event->x();
		mouse_last_y = event->y();
		update();
	}
	else if (event->buttons() & Qt::MidButton) { // when mid(wheel) pressed = panning
		float dx = (float)(event->x() - mouse_last_x);
		float dy = (float)(event->y() - mouse_last_y);
		cur_center_x += dx;
		cur_center_y += dy;

		dx = dx * pixel_len_s * dvr_pixel_num / screen_size;
		dy = dy * pixel_len_s * dvr_pixel_num / screen_size;
		P_screen = P_screen - dx * ws + dy * hs;

		mouse_last_x = event->x();
		mouse_last_y = event->y();
		update();
	}
}

void DVRWidget::wheelEvent(QWheelEvent *event) // when wheel scrolled = zooming
{
	QPoint wd = event->angleDelta();
	int wheel_delta = wd.y(); // only check if positive or negative
	int d = 24;

	if (wheel_delta > 0)
		d *= -1;

	if (L_s + d < N_max / 10 || 4 * N_max < L_s + d)
		return;

	P_screen -= (ws * 7 / 4 + hs) * d / 2;
	cur_center_x = (cur_center_x * L_s / screen_size + d * 7 / 8) * screen_size / (L_s + d);
	cur_center_y = (cur_center_y * L_s / screen_size + d / 2) * screen_size / (L_s + d);
	L_s += d;
	pixel_len_s = L_s / (dvr_pixel_num - 1);
	update();
}

QVector3D DVRWidget::get_arcball_pos(int m_x, int m_y)
{
	float x = m_x - cur_center_x;
	float y = m_y - cur_center_y;
	float r = sqrt(x*x + y*y);
	float arcball_radius = 500.0 * screen_size / L_s;
	if (r > arcball_radius) {
		x = x * arcball_radius / r;
		y = y * arcball_radius / r;
		r = arcball_radius;
	}

	float a = acos(r / arcball_radius);
	float h = arcball_radius * sin(a);

	return QVector3D(x, y, h);
}
