#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include "gdcmImageReader.h"
#include "data_cube.h"
#include "sliceWidget.h"
//TODO_CGIP: add header files here
//ex) #include "cgip_headers/MagicBrush/CgipMagicBrush.h"

class QSignalMapper;
class QLabel;
class QSlider;
class QPushButton;
class QTreeView;
class DVRWidget;
class MainWindow;
class QFileSystemModel;
class QSpacerItem;
class QWidget;
class SliceWidget;
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
	void _set_mode();
	void load_images(int z, int x, int y, int a, int b);
	void z_line_moved(int which);
	void x_line_moved(int which);
	void y_line_moved(int which);
	void update_dvr_slices();
	void toggle_skipping_label();
	void change_function_label();
	void change_function_mode_0();
	void change_function_mode_1();
	void function_start(int slice_type, float x, float y, float z);
	void function_end(int slice_type, float x, float y, float z);
	void update_coord(int slice_type, float x, float y, float z, int v);

private:
	MainWindow *main_window;
	SliceWidget *slice_widget_z, *slice_widget_x, *slice_widget_y;
	DVRWidget *dvr_widget;
	DataCube *data_cube;
	QLabel *create_label(QWidget *sw, int loc_w, int loc_h);
	QLabel *coord_z, *coord_x, *coord_y;
	QLabel *selected, *window_dvr;
	QLabel *skipping_label, *function_label_z, *function_label_x, *function_label_y;
	QPushButton *initBtn, *hideBtn, *toggleLineBtn, *toggleBorderBtn;
	MainWindow *mainWindow;
	QTreeView *tree;
	QFileSystemModel *model;
	QModelIndex *currIdx;
	QList<QString> file_list;
	int view_size_w, view_size_h;
	float slice_thickness;
	QHBoxLayout *create_menubar();
	void add_menubar_button(QHBoxLayout *m_layout, QMenu *m, QString icon_path);
	QWidget *w_browser, *menubar_z, *menubar_x, *menubar_y, *menubar_dvr;

	QHBoxLayout *mainLayout;
	QVBoxLayout *container_3;
	QLabel *blank_dvr;
	QAction *init_DVR_all, *init_DVR_geometry, *init_DVR_windowing;
	QAction *toggle_DVR_mode, *toggle_DVR_skipping, *toggle_DVR_border_line, *toggle_DVR_axial_plane, *toggle_DVR_sagittal_plane, *toggle_DVR_coronal_plane;
	short *data_3d, *mask_3d;
	bool skipping_mode;
	int function_mode, function_started;

	cgip::CgipVolume *cgip_volume;
	cgip::CgipMask *cgip_mask;

	//TODO_CGIP: add pointers for class objects of functions
	//ex) cgip::CgipMagicBrush *cgip_magic_brush;
};

#endif