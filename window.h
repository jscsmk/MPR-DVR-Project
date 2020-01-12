#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include "gdcmImageReader.h"
#include "data_cube.h"
#include "sliceWidget.h"
//#include "screenWidget.h"

class QSignalMapper;
class QLabel;
class QSlider;
class QPushButton;
class QTreeView;
//class GLWidget;
class DVRWidget;
class MainWindow;
class QFileSystemModel;
class QSpacerItem;
class QWidget;
class SliceWidget;
//class ScreenWidget;
class DataCube;
class QMenu;
class QAction;
class QVBoxLayout;
class QHBoxLayout;

class Window : public QWidget
{
	Q_OBJECT

public:
	Window(MainWindow *mw);

protected:
	void keyPressEvent(QKeyEvent *event) override;

signals:
	void myMessage(QString msg);

private slots:
	void get_path();
	void hide_tree();
	void _init_all();
	void _init_geometry();
	void _init_windowing();
	void load_images(int z, int x, int y, int a, int b);
	void z_line_moved(int which);
	void x_line_moved(int which);
	void y_line_moved(int which);
	void update_dvr_slices();

private:
	MainWindow *main_window;
	SliceWidget *slice_widget_z, *slice_widget_x, *slice_widget_y;
	//ScreenWidget *screen_widget;
	//GLWidget *glWidget;
	DVRWidget *dvr_widget;
	DataCube *data_cube;
	QLabel *create_label(QWidget *sw);
	QLabel *selected, *coord_z, *coord_x, *coord_y, *window_z, *window_x, *window_y, *window_dvr, *coord_dvr;
	QPushButton *initBtn, *hideBtn, *toggleLineBtn, *toggleBorderBtn;
	MainWindow *mainWindow;
	QTreeView *tree;
	QFileSystemModel *model;
	QModelIndex *currIdx;
	QList<QString> file_list;
	int view_size;
	float slice_thickness;
	QHBoxLayout *create_menubar();
	void add_menubar_button(QHBoxLayout *m_layout, QMenu *m, QString icon_path);
	QWidget *w_browser, *menubar_z, *menubar_x, *menubar_y, *menubar_dvr;

	QHBoxLayout *mainLayout;
	QVBoxLayout *container_3;
	QLabel *blank_dvr;
	QAction *init_DVR_all, *init_DVR_geometry, *init_DVR_windowing;
	QAction *toggle_DVR_mode, *toggle_DVR_skipping, *toggle_DVR_border_line, *toggle_DVR_axial_plane, *toggle_DVR_sagittal_plane, *toggle_DVR_coronal_plane;
	int *data_3d;
};

#endif