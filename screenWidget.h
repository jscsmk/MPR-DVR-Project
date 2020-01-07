#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QWidget>
#include <Qlabel>
#include "data_cube.h"
#include "window.h"

/* DVR screen widget using CPU
 * may not work at to momnet
 *

class MainWindow;
class QPixmap;
class DataCube;

class ScreenWidget : public QLabel
{
	Q_OBJECT

public:
	ScreenWidget(int s);
	void set_data(DataCube *d);	
	void get_screen();
	void apply_windowing();
	void set_pixmap();	

public slots:	
	void init_geometry();
	void init_windowing();

signals:
	void windowing_info_sig(QString msg);	

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private:
	DataCube *data_cube;
	int *screen;
	unsigned char* windowed_screen;
	QPixmap *blank_img, *loading_img;
	int is_valid, screen_size;
	double pixel_num, rescale_slope, rescale_intercept;
	int mouse_last_x, mouse_last_y;	
	int window_level, window_width;
};
*/
#endif