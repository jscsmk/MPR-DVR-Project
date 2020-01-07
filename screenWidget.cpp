#include "screenwidget.h"
#include "window.h"
#include "data_cube.h"
#include <QMouseEvent>
#include <QCoreApplication>
#include <math.h>
#include <sstream>
#include <string.h>
#include <QLabel>
#include <QPainter>
#include <QLineF>

using namespace std;
#define PI 3.141592

/* DVR screen widget using CPU
 * may not work at to momnet
 * 

ScreenWidget::ScreenWidget(int s)
{	
	is_valid = 0;
	screen_size = s;

	QString blank_path = "C:/vscode_workspace/data/blank_image.png";
	QString loading_path = "C:/vscode_workspace/data/loading_image.png";

	QImage *img;
	img = new QImage();
	blank_img = new QPixmap();
	loading_img = new QPixmap();

	img->load(blank_path);
	*blank_img = QPixmap::fromImage(*img);

	img->load(loading_path);
	*loading_img = QPixmap::fromImage(*img);

	set_pixmap();	
}

void ScreenWidget::set_data(DataCube *d)
{
	is_valid = 1;
	data_cube = d;
	int t;
	tie(t, pixel_num, rescale_slope, rescale_intercept) = data_cube->get_data_info();
	windowed_screen = (unsigned char*)malloc(pixel_num * pixel_num * sizeof(unsigned char));
	window_level = 300;
	window_width = 500;

	get_screen();
}

void ScreenWidget::init_geometry()
{
	data_cube->init_DVR();
	get_screen();
}

void ScreenWidget::get_screen()
{
	// get raw screen from data cube
	screen = data_cube->get_DVR_screen();
	apply_windowing();
}

void ScreenWidget::init_windowing()
{
	window_level = 300;
	window_width = 500;
	apply_windowing();
}

void ScreenWidget::apply_windowing()
{
	// rescale pixel value to HU unit, apply windowing, then convert to 0~255
	for (int i = 0; i < pixel_num * pixel_num; i++) {
		float temp = (float)(screen[i]);

		if (temp == -1) {
			windowed_screen[i] = 255;
		}
		else {
			temp = rescale_slope * temp + rescale_intercept;
			temp -= window_level - window_width / 2;

			if (temp < 0) {
				temp = 0;
			}
			else if (temp > window_width) {
				temp = window_width;
			}
			temp = temp * 255 / window_width;
			windowed_screen[i] = (unsigned char)temp;
		}
	}

	QString msg = "WL: " + QString::number(window_level) + "\nWW: " + QString::number(window_width);
	emit windowing_info_sig(msg);
	set_pixmap();
}

void ScreenWidget::set_pixmap()
{
	QPixmap *img_buffer;
	if (is_valid == 0) {
		// set blank img when not valid
		img_buffer = blank_img;
		*img_buffer = img_buffer->scaledToHeight(screen_size);
		this->setPixmap(*img_buffer);
		return;
	}

	// draw line, then set pixmap of this widget
	QImage *img;
	img = new QImage(windowed_screen, pixel_num, pixel_num, QImage::Format_Indexed8);
	img_buffer = new QPixmap();
	*img_buffer = QPixmap::fromImage(*img);
	*img_buffer = img_buffer->scaledToHeight(screen_size);
	   
	// set pixmap	
	this->setPixmap(*img_buffer);
}

void ScreenWidget::mousePressEvent(QMouseEvent *event)
{
	if (is_valid == 0)
		return;

	mouse_last_x = event->x();
	mouse_last_y = event->y();	
}

void ScreenWidget::mouseMoveEvent(QMouseEvent *event)
{	
	if (is_valid == 0)
		return;	

	float dx = (float)(event->x() - mouse_last_x);
	float dy = (float)(event->y() - mouse_last_y);

	// mouse click event
	if (event->buttons() & Qt::LeftButton) { // when left clicked = rotation			
		dx = dx * pixel_num / screen_size;
		dy = dy * pixel_num / screen_size;
		data_cube->rotate_DVR_screen(dx, dy);

		mouse_last_x = event->x();
		mouse_last_y = event->y();
		get_screen();
	}
	else if (event->buttons() & Qt::RightButton) {// when Right clicked = windowing
		dy *= -1;

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
		apply_windowing();		
	}
	else if (event->buttons() & Qt::MidButton) { // when mid(wheel) pressed == panning		
		dx = dx * pixel_num / screen_size;
		dy = dy * pixel_num / screen_size;

		int moved = data_cube->panning_DVR_screen(dx, dy);
		if (moved == 1) {
			mouse_last_x = event->x();
			mouse_last_y = event->y();
			get_screen();
		}
	}	
}

void ScreenWidget::wheelEvent(QWheelEvent *event)
{
	if (is_valid == 0)
		return;

	QPoint wd = event->angleDelta();
	int wheel_delta = wd.y(); // only check if positive or negative
	int moved = 0;

	if (wheel_delta > 0)
		moved = data_cube->zoom_DVR_screen(-20);
	else
		moved = data_cube->zoom_DVR_screen(20);

	if (moved == 0) // not moved
		return;

	get_screen();
}

*/
