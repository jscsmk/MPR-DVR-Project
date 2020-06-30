#ifndef SLICEWIDGET_H
#define SLICEWIDGET_H

#include <QWidget>
#include <Qlabel>
#include "data_cube.h"
#include "window.h"
#include <QMatrix4x4>

class MainWindow;
class QPixmap;
class DataCube;

class SliceWidget : public QLabel
{
	Q_OBJECT

public:
	SliceWidget(int t, int s_w, int s_h);
	void set_data(DataCube *d);
	void set_mode(int m);
	void get_slice();
	void apply_windowing();
	void set_pixmap();
	void init_windowing(int apply);

public slots:
	void set_windowing(int wl, int ww);
	void toggle_slice_line();
	void toggle_border_line();

signals:
	void coord_info_sig(int slice_type, float coord_x, float coord_y, float coord_z, int pixel_val);
	void mouse_press_sig(int slice_type, float coord_x, float coord_y, float coord_z);
	void mouse_release_sig(int slice_type, float coord_x, float coord_y, float coord_z);
	void mouse_leave_sig(int slice_type);
	void windowing_info_sig(QString msg);
	void line_moved_sig(int which); // which = 0: v-line, 1: h-line, 2: both-line, 3: wheel event
	void zoom_panning_sig();
	void windowing_changed_sig(int wl, int ww);

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void emit_coord_sig(int mouse_x, int mouse_y);

private:
	DataCube *data_cube;
	int *slice_data, *mask_data;
	unsigned char *windowed_slice, *masked_slice;
	QPixmap *blank_img, *loading_img;
	int slice_type, mode, slice_size_w, slice_size_h, mask_count;
	int pixel_num_w, pixel_num_h, rescale_slope, rescale_intercept, pixel_min, pixel_max;
	int is_line_visible;
	int is_line_mouseover_h, is_line_mouseover_v;
	int line_clicked_v, line_clicked_h;
	int mouse_last_x, mouse_last_y;
	int line_x, line_y, line_x_scaled, line_y_scaled;
	float line_angle_rad, mouse_last_a;
	float get_mouse_angle(int mouse_x, int mouse_y);
	int window_level, window_width;
	int window_changed, zoom_changed;
	QPixmap *img_buffer;
	tuple<float, float, float, int> convert_coord(int mouse_x, int mouse_y);
};

#endif