#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include "gdcmImageReader.h"
#include "data_cube.h"
#include "sliceWidget.h"
//TODO_CGIP: add header files here

#include "cgip_header/CgipMagicBrush/CgipMagicBrush.h"
#include "cgip_header/FreeDraw/CgipBrush.h"
#include "cgip_header/FreeDraw/CgipFreeDraw.h"
#include "cgip_header/FreeDraw/CgipCurve.h"
#include "cgip_header/FreeDraw/CgipLiveWire.h"
#include "cgip_header/MPRMod/CgipMPRMod.h"
#include "cgip_header/Common/CgipPoint.h"
#include "cgip_header/Common/CgipVolume.h"
#include "cgip_header/GraphCut/CgipGraphCut2D.h"
#include "cgip_header/GraphCut/CgipGraphCut3D.h"


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
	void load_images(int z, int x, int y, int a, int b);
	void z_line_moved(int which);
	void x_line_moved(int which);
	void y_line_moved(int which);
	void update_dvr_slices();
	void toggle_skipping_label();
	QString get_function_label(int m);
	void change_function_mode_z(int m);
	void change_function_mode_x(int m);
	void change_function_mode_y(int m);
	void change_color_z(int c);
	void change_color_x(int c);
	void change_color_y(int c);
	void update_slice(int slice_type);
	void update_all_slice();
	tuple<int, int> get_function_status(int slice_type);
	void update_coord(int slice_type, float x, float y, float z, int v);
	void mouse_pressed(int slice_type, float x, float y, float z, int click_type);
	void mouse_moved(int slice_type, float x, float y, float z);
	void mouse_released(int slice_type, float x, float y, float z);
	void wheel_changed(int slice_type, int key_type, int dir);

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
	short *data_3d;
	short **mask_3d;
	int mask_count;
	bool skipping_mode;
	int function_mode_z, function_mode_x, function_mode_y, function_started, function_color_z, function_color_x, function_color_y;
	//TODO_CGIP: specify function names
	static const int n_functions = 8;
	QString function_list[n_functions] = { "off", "free draw", "brush", "curve", "live wire" , "magic brush", "graphcut 2d", "graphcut 3d"};
	QString color_list[7] = { "magenta.png", "cyan.png", "yellow.png", "orange.png", "violet.png", "azure.png", "rose.png" };
	QPushButton *color_button_z, *color_button_x, *color_button_y;

	//TODO_CGIP: add pointers for class objects of functions
	/*
	cgip::CgipVolume *cgip_volume;
	cgip::CgipMask **cgip_mask;
	cgip::CgipMagicBrush *cgip_magic_brush;
	*/

	cgip::CgipVolume* cgip_volume;
	cgip::CgipMask** cgip_mask;
	cgip::CgipMask2D** cgip_maskimage;
	cgip::CgipMask2D* cgip_image;
	cgip::CgipVolume* cgip_volume_down;
	cgip::CgipMask** cgip_mask_down;

	//TODO_CGIP: add pointers for class objects of functions
	//ex) cgip::CgipMagicBrush *cgip_magic_brush;

	cgip::CgipMagicBrush* cgip_magic_brush = nullptr;

	cgip::CgipMPRMod* cgip_mprmod;
	cgip::CgipBrush* cgip_brush;
	cgip::CgipFreeDraw* cgip_freedraw;
	cgip::CgipCurve* cgip_curve;
	cgip::CgipLiveWire* cgip_livewire;
	cgip::CgipGraphCut2D *cgip_gc_2d;
	cgip::CgipGraphCut3D *cgip_gc_3d;

	float radius;
	float smooth;
	int action;
};

#endif