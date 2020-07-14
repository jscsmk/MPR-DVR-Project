#include "DVRWidget.h"
#include "data_cube.h"
#include "window.h"
#include "mainwindow.h"
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QSpacerItem>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QFileSystemModel>
#include <QLabel>
#include <QSignalMapper>
#include <QModelIndex>
#include <QVariant>
#include <QTreeView>
#include <QImage>
#include <QPixmap>
#include <QFileInfo>
#include <QPainter>
#include <QTransform>
#include <QMouseEvent>
#include <QPalette>
#include <QAction>
#include <QMenu>
#include <string>

#include "gdcmImageReader.h"
#include "gdcmReader.h"
#include "gdcmTag.h"
#include "gdcmFile.h"
#include "gdcmAttribute.h"
#include "sliceWidget.h"


//TOGO_CGIP: using namespace cgip;
using namespace cgip;

Window::Window(MainWindow *mw)
	: main_window(mw)
{
	/*
	// set black background
	QPalette pal = palette();
	pal.setColor(QPalette::Background, Qt::black);
	this->setAutoFillBackground(true);
	this->setPalette(pal);


	w_browser->setVisible(false);
	*/

	// add XYZ slice widgets
	view_size_h = 600;
	view_size_w = 600 * 7 / 4;
	slice_widget_z = new SliceWidget(0, view_size_w, view_size_h);
	slice_widget_x = new SliceWidget(1, view_size_w, view_size_h);
	slice_widget_y = new SliceWidget(2, view_size_w, view_size_h);
	slice_widget_z->setMouseTracking(true);
	slice_widget_x->setMouseTracking(true);
	slice_widget_y->setMouseTracking(true);
	slice_widget_z->setContentsMargins(0, 0, 0, 20);
	slice_widget_x->setContentsMargins(0, 0, 0, 20);
	data_cube = new DataCube();
	data_3d = NULL;
	mask_3d = NULL;
	dvr_widget = NULL;
	function_mode_z = 0;
	function_mode_x = 0;
	function_mode_y = 0;
	function_started = 0;
	function_color_z = 0;
	function_color_x = 0;
	function_color_y = 0;
	mask_count = 7;

	// add local file browser
	QString root_path = "c:/vscode_workspace";
	model = new QFileSystemModel();
	model->setRootPath(root_path);
	const QModelIndex rootIndex = model->index(QDir::cleanPath(root_path));

	tree = new QTreeView();
	tree->setModel(model);
	tree->setFixedWidth(280);
	tree->setExpandsOnDoubleClick(false);
	tree->setHeaderHidden(true);

	if (rootIndex.isValid())
		tree->setRootIndex(rootIndex);

	for (int i = 1; i < model->columnCount(); ++i)
		tree->hideColumn(i);

	hideBtn = new QPushButton(tr("<"), this);
	hideBtn->setFixedSize(20, 100);


	// add menubar for MPR
	QMenu *toggle_menu_z, *toggle_menu_x, *toggle_menu_y, *init_menu_z, *init_menu_x, *init_menu_y;
	QMenu *function_select_menu_z, *function_select_menu_x, *function_select_menu_y;
	QMenu *color_select_menu_z, *color_select_menu_x, *color_select_menu_y;
	QAction *toggle_slice_line_z, *toggle_slice_line_x, *toggle_slice_line_y;
	QAction *toggle_border_line_z, *toggle_border_line_x, *toggle_border_line_y;
	QAction *function_select_z[n_functions], *function_select_x[n_functions], *function_select_y[n_functions];
	QAction *color_select_z[7], *color_select_x[7], *color_select_y[7];
	QAction *init_all, *init_geometry, *init_windowing;
	QMenu *toggle_menu_dvr, *init_menu_dvr;

	toggle_slice_line_z = new QAction("show/hide slice lines");
	toggle_slice_line_x = new QAction("show/hide slice lines");
	toggle_slice_line_y = new QAction("show/hide slice lines");
	toggle_border_line_z = new QAction("show/hide border lines");
	toggle_border_line_x = new QAction("show/hide border lines");
	toggle_border_line_y = new QAction("show/hide border lines");
	init_all = new QAction("reset all MPR");
	init_geometry = new QAction("reset geometry");
	init_windowing = new QAction("reset window level and width");

	toggle_DVR_border_line = new QAction("show/hide border lines");
	toggle_DVR_axial_plane = new QAction("show/hide axial slice plane");
	toggle_DVR_sagittal_plane = new QAction("show/hide sagittal slice plane");
	toggle_DVR_coronal_plane = new QAction("show/hide coronal slice plane");
	toggle_DVR_mode = new QAction("toggle OTF/MIP mode");
	toggle_DVR_skipping = new QAction("on/off empty-space skipping");
	init_DVR_all = new QAction("reset DVR");
	init_DVR_geometry = new QAction("reset geometry");
	init_DVR_windowing = new QAction("reset window level and width");

	function_select_menu_z = new QMenu();
	function_select_menu_x = new QMenu();
	function_select_menu_y = new QMenu();
	color_select_menu_z = new QMenu();
	color_select_menu_x = new QMenu();
	color_select_menu_y = new QMenu();
	for (int i = 0; i < n_functions; i++) {
		function_select_z[i] = new QAction(function_list[i]);
		function_select_x[i] = new QAction(function_list[i]);
		function_select_y[i] = new QAction(function_list[i]);

		function_select_menu_z->addAction(function_select_z[i]);
		function_select_menu_x->addAction(function_select_x[i]);
		function_select_menu_y->addAction(function_select_y[i]);
	}
	for (int i = 0; i < 7; i++) {
		color_select_z[i] = new QAction("mask " + QString::number(i + 1));
		color_select_x[i] = new QAction("mask " + QString::number(i + 1));
		color_select_y[i] = new QAction("mask " + QString::number(i + 1));

		color_select_z[i]->setIcon(QIcon("icons/" + color_list[i]));
		color_select_x[i]->setIcon(QIcon("icons/" + color_list[i]));
		color_select_y[i]->setIcon(QIcon("icons/" + color_list[i]));

		color_select_menu_z->addAction(color_select_z[i]);
		color_select_menu_x->addAction(color_select_x[i]);
		color_select_menu_y->addAction(color_select_y[i]);
	}

	toggle_menu_z = new QMenu();
	toggle_menu_x = new QMenu();
	toggle_menu_y = new QMenu();
	toggle_menu_z->addAction(toggle_slice_line_z);
	toggle_menu_z->addAction(toggle_border_line_z);
	toggle_menu_x->addAction(toggle_slice_line_x);
	toggle_menu_x->addAction(toggle_border_line_x);
	toggle_menu_y->addAction(toggle_slice_line_y);
	toggle_menu_y->addAction(toggle_border_line_y);

	init_menu_z = new QMenu();
	init_menu_x = new QMenu();
	init_menu_y = new QMenu();
	init_menu_z->addAction(init_all);
	init_menu_z->addAction(init_geometry);
	init_menu_z->addAction(init_windowing);
	init_menu_x->addAction(init_all);
	init_menu_x->addAction(init_geometry);
	init_menu_x->addAction(init_windowing);
	init_menu_y->addAction(init_all);
	init_menu_y->addAction(init_geometry);
	init_menu_y->addAction(init_windowing);

	toggle_menu_dvr = new QMenu();
	init_menu_dvr = new QMenu();
	toggle_menu_dvr->addAction(toggle_DVR_mode);
	toggle_menu_dvr->addAction(toggle_DVR_skipping);
	toggle_menu_dvr->addAction(toggle_DVR_axial_plane);
	toggle_menu_dvr->addAction(toggle_DVR_sagittal_plane);
	toggle_menu_dvr->addAction(toggle_DVR_coronal_plane);
	toggle_menu_dvr->addAction(toggle_DVR_border_line);
	init_menu_dvr->addAction(init_DVR_all);
	init_menu_dvr->addAction(init_DVR_geometry);
	init_menu_dvr->addAction(init_DVR_windowing);

	QHBoxLayout *menubar_layout_z, *menubar_layout_x, *menubar_layout_y, *menubar_layout_dvr;
	menubar_layout_z = create_menubar();
	menubar_layout_x = create_menubar();
	menubar_layout_y = create_menubar();
	menubar_layout_dvr = create_menubar();

	QLabel *name_z, *name_x, *name_y, *name_dvr;
	name_z = new QLabel("  axial");
	name_x = new QLabel("  sagittal");
	name_y = new QLabel("  coronal");
	name_dvr = new QLabel("  DVR");
	name_z->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	name_x->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	name_y->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	name_dvr->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	menubar_layout_z->addWidget(name_z);
	menubar_layout_x->addWidget(name_x);
	menubar_layout_y->addWidget(name_y);
	menubar_layout_dvr->addWidget(name_dvr);

	add_menubar_button(menubar_layout_z, color_select_menu_z, "");
	add_menubar_button(menubar_layout_z, function_select_menu_z, "icons/function.png");
	add_menubar_button(menubar_layout_z, toggle_menu_z, "icons/view.png");
	add_menubar_button(menubar_layout_z, init_menu_z, "icons/reset.png");
	add_menubar_button(menubar_layout_x, color_select_menu_x, "");
	add_menubar_button(menubar_layout_x, function_select_menu_x, "icons/function.png");
	add_menubar_button(menubar_layout_x, toggle_menu_x, "icons/view.png");
	add_menubar_button(menubar_layout_x, init_menu_x, "icons/reset.png");
	add_menubar_button(menubar_layout_y, color_select_menu_y, "");
	add_menubar_button(menubar_layout_y, function_select_menu_y, "icons/function.png");
	add_menubar_button(menubar_layout_y, toggle_menu_y, "icons/view.png");
	add_menubar_button(menubar_layout_y, init_menu_y, "icons/reset.png");
	add_menubar_button(menubar_layout_dvr, toggle_menu_dvr, "icons/view.png");
	add_menubar_button(menubar_layout_dvr, init_menu_dvr, "icons/reset.png");
	color_button_z = dynamic_cast<QPushButton*>(menubar_layout_z->itemAt(1)->widget());
	color_button_x = dynamic_cast<QPushButton*>(menubar_layout_x->itemAt(1)->widget());
	color_button_y = dynamic_cast<QPushButton*>(menubar_layout_y->itemAt(1)->widget());
	color_button_z->setIcon(QIcon("icons/" + color_list[function_color_z]));
	color_button_x->setIcon(QIcon("icons/" + color_list[function_color_x]));
	color_button_y->setIcon(QIcon("icons/" + color_list[function_color_y]));

	menubar_z = new QWidget();
	menubar_x = new QWidget();
	menubar_y = new QWidget();
	menubar_dvr = new QWidget();
	menubar_z->setLayout(menubar_layout_z);
	menubar_x->setLayout(menubar_layout_x);
	menubar_y->setLayout(menubar_layout_y);
	menubar_dvr->setLayout(menubar_layout_dvr);

	menubar_z->setEnabled(false);
	menubar_x->setEnabled(false);
	menubar_y->setEnabled(false);
	menubar_dvr->setEnabled(false);


	// add labels
	selected = new QLabel("Path: ");
	selected->setAlignment(Qt::AlignLeft);;

	QLabel *window_z, *window_x, *window_y;
	coord_z = create_label(slice_widget_z, view_size_w - 210, view_size_h - 40);
	coord_x = create_label(slice_widget_x, view_size_w - 210, view_size_h - 40);
	coord_y = create_label(slice_widget_y, view_size_w - 210, view_size_h - 40);
	window_z = create_label(slice_widget_z, 10, view_size_h - 40);
	window_x = create_label(slice_widget_x, 10, view_size_h - 40);
	window_y = create_label(slice_widget_y, 10, view_size_h - 40);
	function_label_z = create_label(slice_widget_z, 10, 10);
	function_label_x = create_label(slice_widget_x, 10, 10);
	function_label_y = create_label(slice_widget_y, 10, 10);
	function_label_z->setText(get_function_label(function_mode_z));
	function_label_x->setText(get_function_label(function_mode_x));
	function_label_y->setText(get_function_label(function_mode_y));


	// add connections
	connect(tree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(get_path()));
	connect(hideBtn, &QPushButton::clicked, this, &Window::hide_tree);

	connect(toggle_slice_line_z, &QAction::triggered, slice_widget_z, &SliceWidget::toggle_slice_line);
	connect(toggle_slice_line_x, &QAction::triggered, slice_widget_x, &SliceWidget::toggle_slice_line);
	connect(toggle_slice_line_y, &QAction::triggered, slice_widget_y, &SliceWidget::toggle_slice_line);
	connect(toggle_border_line_z, &QAction::triggered, slice_widget_z, &SliceWidget::toggle_border_line);
	connect(toggle_border_line_x, &QAction::triggered, slice_widget_x, &SliceWidget::toggle_border_line);
	connect(toggle_border_line_y, &QAction::triggered, slice_widget_y, &SliceWidget::toggle_border_line);

	connect(init_all, &QAction::triggered, this, &Window::_init_all);
	connect(init_geometry, &QAction::triggered, this, &Window::_init_geometry);
	connect(init_windowing, &QAction::triggered, this, &Window::_init_windowing);

	QSignalMapper *function_mapper_z = new QSignalMapper(this);
	QSignalMapper *function_mapper_x = new QSignalMapper(this);
	QSignalMapper *function_mapper_y = new QSignalMapper(this);
	QSignalMapper *color_mapper_z = new QSignalMapper(this);
	QSignalMapper *color_mapper_x = new QSignalMapper(this);
	QSignalMapper *color_mapper_y = new QSignalMapper(this);
	connect(function_mapper_z, SIGNAL(mapped(int)), this, SLOT(change_function_mode_z(int)));
	connect(function_mapper_x, SIGNAL(mapped(int)), this, SLOT(change_function_mode_x(int)));
	connect(function_mapper_y, SIGNAL(mapped(int)), this, SLOT(change_function_mode_y(int)));
	connect(color_mapper_z, SIGNAL(mapped(int)), this, SLOT(change_color_z(int)));
	connect(color_mapper_x, SIGNAL(mapped(int)), this, SLOT(change_color_x(int)));
	connect(color_mapper_y, SIGNAL(mapped(int)), this, SLOT(change_color_y(int)));
	for (int i = 0; i < n_functions; i++) {
		connect(function_select_z[i], SIGNAL(triggered()), function_mapper_z, SLOT(map()));
		connect(function_select_x[i], SIGNAL(triggered()), function_mapper_x, SLOT(map()));
		connect(function_select_y[i], SIGNAL(triggered()), function_mapper_y, SLOT(map()));

		function_mapper_z->setMapping(function_select_z[i], i);
		function_mapper_x->setMapping(function_select_x[i], i);
		function_mapper_y->setMapping(function_select_y[i], i);
	}
	for (int i = 0; i < 7; i++) {
		connect(color_select_z[i], SIGNAL(triggered()), color_mapper_z, SLOT(map()));
		connect(color_select_x[i], SIGNAL(triggered()), color_mapper_x, SLOT(map()));
		connect(color_select_y[i], SIGNAL(triggered()), color_mapper_y, SLOT(map()));

		color_mapper_z->setMapping(color_select_z[i], i);
		color_mapper_x->setMapping(color_select_x[i], i);
		color_mapper_y->setMapping(color_select_y[i], i);
	}

	connect(slice_widget_z, SIGNAL(coord_info_sig(int, float, float, float, int)), this, SLOT(update_coord(int, float, float, float, int)));
	connect(slice_widget_x, SIGNAL(coord_info_sig(int, float, float, float, int)), this, SLOT(update_coord(int, float, float, float, int)));
	connect(slice_widget_y, SIGNAL(coord_info_sig(int, float, float, float, int)), this, SLOT(update_coord(int, float, float, float, int)));
	connect(slice_widget_z, SIGNAL(mouse_press_sig(int, float, float, float, int)), this, SLOT(mouse_pressed(int, float, float, float, int)));
	connect(slice_widget_x, SIGNAL(mouse_press_sig(int, float, float, float, int)), this, SLOT(mouse_pressed(int, float, float, float, int)));
	connect(slice_widget_y, SIGNAL(mouse_press_sig(int, float, float, float, int)), this, SLOT(mouse_pressed(int, float, float, float, int)));
	connect(slice_widget_z, SIGNAL(mouse_release_sig(int, float, float, float)), this, SLOT(mouse_released(int, float, float, float)));
	connect(slice_widget_x, SIGNAL(mouse_release_sig(int, float, float, float)), this, SLOT(mouse_released(int, float, float, float)));
	connect(slice_widget_y, SIGNAL(mouse_release_sig(int, float, float, float)), this, SLOT(mouse_released(int, float, float, float)));
	connect(slice_widget_z, SIGNAL(wheel_sig(int, int, int)), this, SLOT(wheel_changed(int, int, int)));
	connect(slice_widget_x, SIGNAL(wheel_sig(int, int, int)), this, SLOT(wheel_changed(int, int, int)));
	connect(slice_widget_y, SIGNAL(wheel_sig(int, int, int)), this, SLOT(wheel_changed(int, int, int)));

	connect(slice_widget_z, SIGNAL(windowing_info_sig(QString)), window_z, SLOT(setText(QString)));
	connect(slice_widget_x, SIGNAL(windowing_info_sig(QString)), window_x, SLOT(setText(QString)));
	connect(slice_widget_y, SIGNAL(windowing_info_sig(QString)), window_y, SLOT(setText(QString)));

	connect(slice_widget_z, SIGNAL(line_moved_sig(int)), this, SLOT(z_line_moved(int)));
	connect(slice_widget_x, SIGNAL(line_moved_sig(int)), this, SLOT(x_line_moved(int)));
	connect(slice_widget_y, SIGNAL(line_moved_sig(int)), this, SLOT(y_line_moved(int)));

	connect(slice_widget_z, SIGNAL(zoom_panning_sig()), this, SLOT(update_dvr_slices()));
	connect(slice_widget_x, SIGNAL(zoom_panning_sig()), this, SLOT(update_dvr_slices()));
	connect(slice_widget_y, SIGNAL(zoom_panning_sig()), this, SLOT(update_dvr_slices()));

	connect(slice_widget_z, SIGNAL(windowing_changed_sig(int, int)), slice_widget_x, SLOT(set_windowing(int, int)));
	connect(slice_widget_z, SIGNAL(windowing_changed_sig(int, int)), slice_widget_y, SLOT(set_windowing(int, int)));
	connect(slice_widget_x, SIGNAL(windowing_changed_sig(int, int)), slice_widget_y, SLOT(set_windowing(int, int)));
	connect(slice_widget_x, SIGNAL(windowing_changed_sig(int, int)), slice_widget_z, SLOT(set_windowing(int, int)));
	connect(slice_widget_y, SIGNAL(windowing_changed_sig(int, int)), slice_widget_z, SLOT(set_windowing(int, int)));
	connect(slice_widget_y, SIGNAL(windowing_changed_sig(int, int)), slice_widget_x, SLOT(set_windowing(int, int)));


	// set layout
	mainLayout = new QHBoxLayout;
	QVBoxLayout *container_1 = new QVBoxLayout;
	QVBoxLayout *container_2 = new QVBoxLayout;
	container_3 = new QVBoxLayout;

	w_browser = new QWidget;

	container_1->addWidget(selected);
	container_1->addWidget(tree);
	container_1->setMargin(0);
	w_browser->setLayout(container_1);
	w_browser->setFixedWidth(280);

	container_2->addWidget(menubar_z);
	container_2->addWidget(slice_widget_z);
	container_2->addWidget(menubar_y);
	container_2->addWidget(slice_widget_y);
	container_2->setAlignment(Qt::AlignHCenter);
	container_2->setSpacing(0);
	container_2->setMargin(0);

	// add blank image temporalily, instead of DVR widget
	QString blank_path = "images/blank_image.png";
	QImage *img = new QImage(blank_path);
	QPixmap *blank_img = new QPixmap(QPixmap::fromImage(*img));
	*blank_img = blank_img->scaled(view_size_w, view_size_h);

	blank_dvr = new QLabel();
	blank_dvr->setPixmap(*blank_img);

	container_3->addWidget(menubar_x);
	container_3->addWidget(slice_widget_x);
	container_3->addWidget(menubar_dvr);
	container_3->addWidget(blank_dvr);
	container_3->setAlignment(Qt::AlignHCenter);
	container_3->setSpacing(0);
	container_3->setMargin(0);

	mainLayout->addWidget(w_browser);
	mainLayout->addWidget(hideBtn);
	mainLayout->addLayout(container_2);
	mainLayout->addLayout(container_3);

	setLayout(mainLayout);
	this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	setWindowTitle(tr("Hello GL"));
}

void Window::hide_tree()
{
	if (w_browser->isVisible()) {
		w_browser->setVisible(false);
		hideBtn->setText(">");
	}
	else {
		w_browser->setVisible(true);
		hideBtn->setText("<");
	}

	this->adjustSize();
	main_window->adjustSize();
}

void Window::_init_geometry()
{
	data_cube->init_MPR();

	slice_widget_z->get_slice();
	slice_widget_x->get_slice();
	slice_widget_y->get_slice();

	update_dvr_slices();
}
void Window::_init_windowing()
{
	slice_widget_z->init_windowing(1);
	slice_widget_x->init_windowing(1);
	slice_widget_y->init_windowing(1);
}
void Window::_init_all()
{
	slice_widget_z->init_windowing(0);
	slice_widget_x->init_windowing(0);
	slice_widget_y->init_windowing(0);

	_init_geometry();
}

void Window::change_function_mode_z(int m)
{
	if (function_mode_z == m)
		return;

	function_mode_z = m;
	function_label_z->setText(get_function_label(function_mode_z));
	slice_widget_z->set_mode(function_mode_z);

	// In case CgipMagicBrush
	if (function_mode_z == 5) {
		change_function_mode_x(5);
		change_function_mode_y(5);

		if (cgip_magic_brush == nullptr) {
			int this_function_mode, this_function_color;
			tie(this_function_mode, this_function_color) = get_function_status(0);
			printf("num: %d ", this_function_color);
			cgip_magic_brush = new CgipMagicBrush(30.0, 10.0, cgip_volume, cgip_mask[this_function_color]);
			printf("Created Magic Brush\n");
		}
	}
}
void Window::change_function_mode_x(int m)
{
	if (function_mode_x == m)
		return;

	function_mode_x = m;
	function_label_x->setText(get_function_label(function_mode_x));
	slice_widget_x->set_mode(function_mode_x);
}
void Window::change_function_mode_y(int m)
{
	if (function_mode_y == m)
		return;

	function_mode_y = m;
	function_label_y->setText(get_function_label(function_mode_y));
	slice_widget_y->set_mode(function_mode_y);
}
void Window::change_color_z(int c)
{
	if (function_color_z == c)
		return;

	function_color_z = c;
	color_button_z->setIcon(QIcon("icons/" + color_list[function_color_z]));
}
void Window::change_color_x(int c)
{
	if (function_color_x == c)
		return;

	function_color_x = c;
	color_button_x->setIcon(QIcon("icons/" + color_list[function_color_x]));
}
void Window::change_color_y(int c)
{
	if (function_color_y == c)
		return;

	function_color_y = c;
	color_button_y->setIcon(QIcon("icons/" + color_list[function_color_y]));
}

void Window::get_path()
{
	QModelIndex index = tree->currentIndex();
	QString cur_path = model->filePath(index);
	QModelIndex p_idx = model->index(cur_path);

	/*
	if (!tree->isExpanded(index)) {
		connect(tree, SIGNAL(expanded(index)), loop_1, SLOT(quit()));
		connect(model, SIGNAL(directoryLoaded(QString)), loop_1, SLOT(quit()));

		tree->expand(index);
		loop_1->exec();
		tree->collapse(index);
	}
	*/

	int num_rows, img_count, img_w, img_h, diff_count, rescale_intercept, rescale_slope;
	num_rows = model->rowCount(p_idx);
	diff_count = 0;
	img_count = 0;
	img_w = -1;
	img_h = -1;

	selected->setText("Path: " + cur_path);
	file_list = {};

	for (int i = 0; i < num_rows; ++i) {
		QModelIndex childIndex = model->index(i, 0, p_idx);
		QString file_name = model->data(childIndex).toString();
		QFileInfo fi(file_name);
		QString file_path = cur_path + "/" + file_name;

		if (fi.suffix() == "dcm" || fi.suffix() == "DCM") {
			file_list.append(file_path);
			img_count++;

			// get image width, height, and rescale values from DICOM tag
			// rescaling to HU unit: see https://dicom.innolitics.com/ciods/ct-image/ct-image/00281052)
			gdcm::Reader reader;
			reader.SetFileName(file_path.toStdString().c_str());
			reader.Read();
			gdcm::File &file = reader.GetFile();
			gdcm::DataSet &ds = file.GetDataSet();
			gdcm::Attribute<0x0028, 0x0010> at_rows;
			gdcm::Attribute<0x0028, 0x0011> at_cols;
			gdcm::Attribute<0x0028, 0x1052> at_rescale_intercept;
			gdcm::Attribute<0x0028, 0x1053> at_rescale_slope;
			gdcm::Attribute<0x0028, 0x0030> at_pixel_spacing;
			gdcm::Attribute<0x0018, 0x0050> at_slice_thickness;
			at_rows.SetFromDataElement(ds.GetDataElement(at_rows.GetTag()));
			at_cols.SetFromDataElement(ds.GetDataElement(at_cols.GetTag()));
			at_rescale_intercept.SetFromDataElement(ds.GetDataElement(at_rescale_intercept.GetTag()));
			at_rescale_slope.SetFromDataElement(ds.GetDataElement(at_rescale_slope.GetTag()));
			at_pixel_spacing.SetFromDataElement(ds.GetDataElement(at_pixel_spacing.GetTag()));
			at_slice_thickness.SetFromDataElement(ds.GetDataElement(at_slice_thickness.GetTag()));

			int w, h, a, b;
			h = at_rows.GetValue();
			w = at_cols.GetValue();
			a = at_rescale_slope.GetValue();
			b = at_rescale_intercept.GetValue();

			//float pixel_slice_ratio = ((float)at_slice_thickness.GetValue()) / at_pixel_spacing.GetValue();
			//selected->setText(QString::number(at_pixel_spacing.GetValue()));

			slice_thickness = at_slice_thickness.GetValue();
			//auto ps = at_pixel_spacing.GetValues();
			//selected->setText(QString::number(at_slice_thickness.GetValue()) + ", " + QString::number(*ps) + ", " + QString::number(*(ps+1)));
			//selected->setText(QString::number(a) + ", " + QString::number(b));


			if (img_w < 0 || img_h < 0) {
				img_w = w;
				img_h = h;
				rescale_intercept = b;
				rescale_slope = a;
			}
			else if (img_w != w || img_h != h || rescale_intercept != b || rescale_slope != a) {
				// if some images have different h,w,a,b: wrong!
				diff_count++;
			}
		}
		/*else if (fi.suffix() == "jpg" || fi.suffix() == "png") {
			file_list.append(file_path);
			img_count++;

			QImage image;
			int w, h;
			image.load(file_path);
			w = image.width();
			h = image.height();
			if (img_w == 0 || img_h == 0) {
				img_w = w;
				img_h = h;
			}
			else if (img_w != w || img_h != h) {
				diff_wh++;
			}
		}*/
	}

	if (img_count == 0 || diff_count > 0) { // invalid case
		menubar_z->setEnabled(false);
		menubar_x->setEnabled(false);
		menubar_y->setEnabled(false);
		menubar_dvr->setEnabled(false);
	}
	else {
		load_images(img_count, img_w, img_h, rescale_slope, rescale_intercept);

		menubar_z->setEnabled(true);
		menubar_x->setEnabled(true);
		menubar_y->setEnabled(true);
		menubar_dvr->setEnabled(true);
	}
}

void Window::load_images(int z, int x, int y, int a, int b)
{
	free(data_3d);
	data_3d = (short*)malloc(z * x * y * sizeof(short));

	int p_min, p_max;
	p_min = 0;
	p_max = 0;

	for (int i = 0; i < z; i++) {
		QString file_path = file_list[i];
		QFileInfo fi(file_path);

		if (fi.suffix() == "dcm" || fi.suffix() == "DCM") {
			gdcm::ImageReader reader;
			reader.SetFileName(file_path.toStdString().c_str());
			reader.Read();

			gdcm::Image &gimage = reader.GetImage();
			std::vector<char> vbuffer;
			vbuffer.resize(gimage.GetBufferLength());
			char *buffer = &vbuffer[0];
			gimage.GetBuffer(buffer);

			// this only works with UINT16
			// TODO: change this part to support other data types of DICOM
			short *buffer16 = (short*)buffer;
			for (int j = 0; j < y; j++)	{
				for (int k = 0; k < x; k++)	{
					short stored_pixel_value = *buffer16;
					data_3d[x*y*i + x * j + k] = (short)stored_pixel_value;
					buffer16++;

					if (p_max < stored_pixel_value)
						p_max = stored_pixel_value;
					if (p_min > stored_pixel_value)
						p_min = stored_pixel_value;
				}
			}
		}
		/*else { // .jpg or .png file
			image.load(file_path);
			const unsigned char *bit_data = image.bits();
			for (int j = 0; j < y; j++)
			{
				for (int k = 0; k < x; k++)
				{
					data_3d[x*y*i + x * j + k] = (int)(bit_data[k]);
				}
				bit_data += x;
			}
		}*/
	}
	//selected->setText(QString::number(p_min) + ", " + QString::number(p_max));


	// initialize mask_3d
	if (mask_3d != NULL) {
		for (int m = 0; m < mask_count; m++)
			free(mask_3d[m]);
	}
	free(mask_3d);
	mask_3d = (short **)malloc(mask_count * sizeof(short *));
	for (int m = 0; m < mask_count; m++) {
		mask_3d[m] = (short *)malloc(z * x * y * sizeof(short));
		for (int i = 0; i < x*y*z; i++)
			mask_3d[m][i] = 0;

		/*for (int i = 0; i < z; i++) {
			for (int j = 0; j < y; j++) {
				for (int k = 0; k < x; k++) {
					mask_3d[m][x*y*i + x * j + k] = 100 < i && i < 150 && 30*m < j && j < 30+30*m && 30*m < k && k < 30*m+30 ? 1 : 0;
				}
			}
		}*/
	}

	int slice_pixel_num_h = 512;
	int slice_pixel_num_w = 512 * 7 / 4;
	int dvr_pixel_num = 512;
	float unit_ray_len = 1;
	skipping_mode = true;
	QString skip_text = "empty-space skipping: ON";


	//TODO_CGIP: add class objects here
	/*
	cgip_volume = new CgipVolume(x, y, z, data_3d);
	cgip_mask = (CgipMask **)malloc(mask_count * sizeof(CgipMask *));
	for (int m = 0; m < mask_count; m++)
		cgip_mask[m] = new CgipMask(x, y, z, mask_3d[m]);

	cgip_volume->setSpacingX(1);
	cgip_volume->setSpacingY(1);
	cgip_volume->setSpacingZ(slice_thickness);
	cgip_magic_brush = new CgipMagicBrush(50, 1, cgip_volume, cgip_mask);
	*/

	cgip_volume = new CgipVolume(x, y, z, data_3d);
	cgip_mask = (CgipMask**)malloc(mask_count * sizeof(CgipMask*));
	for (int m = 0; m < mask_count; m++)
		cgip_mask[m] = new CgipMask(x, y, z, mask_3d[m]);

	radius = 10;
	action = 1;

	cgip_volume->setSpacingX(1);
	cgip_volume->setSpacingY(1);
	cgip_volume->setSpacingZ(slice_thickness);

	// set data_cube and slice_widget
	data_cube->set_data(data_3d, x, y, z, slice_pixel_num_w, slice_pixel_num_h, a, b, slice_thickness, p_min, p_max);
	data_cube->set_mask(mask_3d, mask_count);
	slice_widget_z->set_data(data_cube);
	slice_widget_x->set_data(data_cube);
	slice_widget_y->set_data(data_cube);


	// set dvr widget
	if (!dvr_widget) {
		container_3->removeItem(container_3->itemAt(3));
		blank_dvr->setVisible(false);

		dvr_widget = new DVRWidget(data_cube, unit_ray_len, a, b, dvr_pixel_num, view_size_h);
		dvr_widget->setMouseTracking(true);

		window_dvr = create_label(dvr_widget, 10, view_size_h - 40);
		skipping_label = create_label(dvr_widget, 10, 10);
		skipping_label->move(10, 10);
		skipping_label->setText(skip_text);

		connect(dvr_widget, SIGNAL(windowing_info_sig(QString)), window_dvr, SLOT(setText(QString)));
		connect(init_DVR_all, &QAction::triggered, dvr_widget, &DVRWidget::init_all);
		connect(init_DVR_geometry, &QAction::triggered, dvr_widget, &DVRWidget::init_geometry);
		connect(init_DVR_windowing, &QAction::triggered, dvr_widget, &DVRWidget::init_windowing);
		connect(toggle_DVR_mode, &QAction::triggered, dvr_widget, &DVRWidget::toggle_mode);
		connect(toggle_DVR_skipping, &QAction::triggered, dvr_widget, &DVRWidget::toggle_skipping);
		connect(toggle_DVR_skipping, &QAction::triggered, this, &Window::toggle_skipping_label);
		connect(toggle_DVR_border_line, &QAction::triggered, dvr_widget, &DVRWidget::toggle_border_line);
		connect(toggle_DVR_axial_plane, &QAction::triggered, dvr_widget, &DVRWidget::toggle_axial_slice);
		connect(toggle_DVR_sagittal_plane, &QAction::triggered, dvr_widget, &DVRWidget::toggle_sagittal_slice);
		connect(toggle_DVR_coronal_plane, &QAction::triggered, dvr_widget, &DVRWidget::toggle_coronal_slice);

		container_3->addWidget(dvr_widget);
	}
	else {
		dvr_widget->set_data(data_cube, unit_ray_len, a, b);
		window_dvr->setParent(dvr_widget);
		window_dvr->move(10, view_size_h - 40);
		skipping_label->setParent(dvr_widget);
		skipping_label->move(10, 10);
		skipping_label->setText(skip_text);
	}
}

QLabel *Window::create_label(QWidget *sw, int loc_w, int loc_h)
{
	QLabel *label = new QLabel;
	label->setFixedSize(250, 30);
	label->setParent(sw);
	label->move(loc_w, loc_h);

	QPalette pal = palette();
	pal.setColor(label->foregroundRole(), Qt::white);
	label->setPalette(pal);

	return label;
}

QHBoxLayout *Window::create_menubar()
{
	QHBoxLayout *menubar_layout = new QHBoxLayout;
	menubar_layout->setAlignment(Qt::AlignRight);
	menubar_layout->setSpacing(0);
	menubar_layout->setMargin(0);

	return menubar_layout;
}

void Window::add_menubar_button(QHBoxLayout *m_layout, QMenu *m, QString icon_path="")
{
	QPushButton *new_button = new QPushButton;
	new_button->setStyleSheet("text-align:left;");
	if (icon_path != "")
		new_button->setIcon(QIcon(icon_path));
	new_button->setIconSize(QSize(20, 20));
	new_button->setMenu(m);

	m_layout->addWidget(new_button);
}

void Window::z_line_moved(int which)
{
	if (which == 2) { // both moved
		slice_widget_x->get_slice();
		slice_widget_y->get_slice();
	}
	else if (which == 0) { // v line moved
		slice_widget_x->get_slice();
		slice_widget_y->set_pixmap();
	}
	else if (which == 1) { // h line moved
		slice_widget_x->set_pixmap();
		slice_widget_y->get_slice();
	}
	else { // wheel event
		slice_widget_x->set_pixmap();
		slice_widget_y->set_pixmap();
	}

	update_dvr_slices();
}
void Window::x_line_moved(int which)
{
	if (which == 2) { // both moved
		slice_widget_z->get_slice();
		slice_widget_y->get_slice();
	}
	else if (which == 0) { // v line moved
		slice_widget_z->get_slice();
		slice_widget_y->set_pixmap();
	}
	else if (which == 1) { // h line moved
		slice_widget_z->set_pixmap();
		slice_widget_y->get_slice();
	}
	else { // wheel event
		slice_widget_z->set_pixmap();
		slice_widget_y->set_pixmap();
	}

	update_dvr_slices();
}
void Window::y_line_moved(int which)
{
	if (which == 2) { // both moved
		slice_widget_x->get_slice();
		slice_widget_z->get_slice();
	}
	else if (which == 0) { // v line moved
		slice_widget_x->get_slice();
		slice_widget_z->set_pixmap();
	}
	else if (which == 1) { // h line moved
		slice_widget_x->set_pixmap();
		slice_widget_z->get_slice();
	}
	else { // wheel event
		slice_widget_x->set_pixmap();
		slice_widget_z->set_pixmap();
	}

	update_dvr_slices();
}
void Window::update_slice(int slice_type)
{
	if (slice_type == 0)
		slice_widget_z->get_slice();
	else if (slice_type == 1)
		slice_widget_x->get_slice();
	else // slice_type == 2
		slice_widget_y->get_slice();
}
void Window::update_all_slice()
{
	slice_widget_z->get_slice();
	slice_widget_x->get_slice();
	slice_widget_y->get_slice();
}
void Window::update_dvr_slices()
{
	if (dvr_widget)
		dvr_widget->get_slice_info();
}

void Window::toggle_skipping_label()
{
	QString skip_text;
	skip_text = skipping_mode ? "OFF" : "ON";
	skip_text = "empty-space skipping: " + skip_text;
	skipping_label->setText(skip_text);
	skipping_mode = skipping_mode ? false : true;
}
QString Window::get_function_label(int m)
{
	QString mode_text = "current function: ";
	mode_text += function_list[m];
	if (m != 0)
		mode_text += function_started ? " - started" : " - terminated";

	return QString(mode_text);
}

void Window::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
		close();
	else
		QWidget::keyPressEvent(e);
}

tuple<int, int> Window::get_function_status(int slice_type)
{
	int this_function_mode, this_function_color;
	if (slice_type == 0) {
		this_function_mode = function_mode_z;
		this_function_color = function_color_z;
		function_label_z->setText(get_function_label(function_mode_z));
	}
	else if (slice_type == 1) {
		this_function_mode = function_mode_x;
		this_function_color = function_color_x;
		function_label_x->setText(get_function_label(function_mode_x));
	}
	else {
		this_function_mode = function_mode_y;
		this_function_color = function_color_y;
		function_label_y->setText(get_function_label(function_mode_y));
	}

	return { this_function_mode, this_function_color };
}

void Window::update_coord(int slice_type, float x, float y, float z, int v)
{
	QString msg;
	msg = "Coord: (" + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + ")\nIntensity(HU): " + QString::number(v);

	if (function_started == 1) {
		mouse_moved(slice_type, x, y, z);
	}

	if (slice_type == 0)
		coord_z->setText(msg);
	else if (slice_type == 1)
		coord_x->setText(msg);
	else
		coord_y->setText(msg);
}
void Window::mouse_pressed(int slice_type, float x, float y, float z, int click_type)
{
	int this_function_mode, this_function_color;
	tie(this_function_mode, this_function_color) = get_function_status(slice_type);

	//TODO_CGIP: add mouse press event here
	/*
	Tip1: CgipPoint cur_point(x, y, z / slice_thickness);
	Tip2: change function_started to 1 when started
	Tip3: click_type = 1 is left, 2 is right click

	if (this_function_mode == 1) {

	}
	else if (this_function_mode == 2) {

	}
	*/

	// init
	QVector3D s, r, d;
	int w, h;
	float pl;
	if (this_function_mode == 1 || this_function_mode == 2 || (this_function_mode == 3 && function_started == 0) || (this_function_mode == 4 && function_started == 0)) {
		tie(s, r, d, w, h, pl) = data_cube->get_MPR_info(slice_type);
		cgip_mprmod = new CgipMPRMod(CgipPoint(s.x(), s.y(), s.z()), w, h, pl, CgipPoint(r.x(), r.y(), r.z()), CgipPoint(d.x(), d.y(), d.z()), 1, 1, 3);
	}

	if (this_function_mode == 1) { // free draw
		cgip_freedraw = new CgipFreeDraw(action, cgip_mask[this_function_color]);
		cgip_freedraw->init_mpr(cgip_mprmod);
		cgip_freedraw->startBall(CgipPoint(x, y, z / slice_thickness));
		function_started = 1;
	}
	else if (this_function_mode == 2) { // brush
		cgip_brush = new CgipBrush(action, radius, cgip_mask[this_function_color]);
		cgip_brush->init_mpr(cgip_mprmod);
		cgip_brush->startBall(CgipPoint(x, y, z / slice_thickness));
		function_started = 1;
	}
	else if (this_function_mode == 3) { // curve
		if (function_started == 0) { // first pressed
			if (click_type == 1) {
				cgip_curve = new CgipCurve(action, cgip_mask[this_function_color]);
				cgip_curve->init_mpr(cgip_mprmod);
				cgip_curve->startCurve(CgipPoint(x, y, z / slice_thickness));
				function_started = 1;
			}
		}
		else {
			if (click_type == 1) {
				cgip_curve->addPoints(CgipPoint(x, y, z / slice_thickness));
			}
			else if (click_type == 2) {
				cgip_curve->endCurve(CgipPoint(x, y, z / slice_thickness));
				function_started = 0;
			}
		}
	}
	else if (this_function_mode == 4) { // livewire
		if (function_started == 0) { // first pressed
			cgip_livewire = new CgipLiveWire(action, cgip_volume, cgip_mask[this_function_color]);
			cgip_livewire->init_mpr(cgip_mprmod);
			cgip_livewire->startWire(CgipPoint(x, y, z / slice_thickness));
			function_started = 1;
		}
		else {
			if (click_type == 1) {
				cgip_livewire->addSeed(CgipPoint(x, y, z / slice_thickness));
			}
			else if (click_type == 2) {
				cgip_livewire->endWire(CgipPoint(x, y, z / slice_thickness));
				function_started = 0;
			}
		}
	}
	else if (this_function_mode == 5) { // Magic brush
		if (cgip_magic_brush) {
			cgip_magic_brush->startBrush(CgipPoint(x, y, z / slice_thickness));
			update_all_slice();
			function_started = 1;

		}
	}
}
void Window::mouse_moved(int slice_type, float x, float y, float z)
{
	int this_function_mode, this_function_color;
	tie(this_function_mode, this_function_color) = get_function_status(slice_type);
	//TODO_CGIP: add mouse move event here
	/*
	Tip1: CgipPoint cur_point(x, y, z / slice_thickness);
	Tip2: this is called only if function_started == 1

	if (function_mode == 1) {

	}
	else if (function_mode == 2) {

	}
	*/
	if (this_function_mode == 1) { // free draw
		cgip_freedraw->moveBall(CgipPoint(x, y, z / slice_thickness));
	}
	else if (this_function_mode == 2) { // brush
		cgip_brush->moveBall(CgipPoint(x, y, z / slice_thickness));
		update_slice(slice_type);
	}
	else if (this_function_mode == 3) { // curve
		cgip_curve->moveCurve(CgipPoint(x, y, z / slice_thickness));
	}
	else if (this_function_mode == 4) { // livewire
		cgip_livewire->moveWire(CgipPoint(x, y, z / slice_thickness));
	}
	else if (this_function_mode == 5) { // Magic brush
		if (cgip_magic_brush) {
			cgip_magic_brush->moveBrush(CgipPoint(x, y, z / slice_thickness));
			update_all_slice();
		}
	}
}
void Window::mouse_released(int slice_type, float x, float y, float z)
{
	int this_function_mode, this_function_color;
	tie(this_function_mode, this_function_color) = get_function_status(slice_type);

	//TODO_CGIP: add mouse release event here
	/*
	Tip1: CgipPoint cur_point(x, y, z / slice_thickness);
	Tip2: change function_started to 0 when terminated
	TIP3: to update slices, do "update_all_slice();"

	CgipMask this_mask = cgip_mask[this_function_color];

	if (this_function_mode == 1) {

	}
	else if (this_function_mode == 2) {

	}
	*/
	if (this_function_mode == 1) { // free draw
		cgip_freedraw->endBall(CgipPoint(x, y, z / slice_thickness));

		function_started = 0;
	}
	else if (this_function_mode == 2) { // brush
		cgip_brush->endBall(CgipPoint(x, y, z / slice_thickness));

		function_started = 0;
	}
	else if (this_function_mode == 5) { // Magic brush
		if (cgip_magic_brush) {
			cgip_magic_brush->endBrush(CgipPoint(x, y, z / slice_thickness));
			function_started = 0;
		}
	}

	update_all_slice();
}
void Window::wheel_changed(int slice_type, int key_type, int dir)
{
	//TODO_CGIP: add mouse wheel event here
	/*
	Tip1: key_type = 1 is Ctrl key, key_type = 2 is Shift key
	Tip2: dir is either 1 or -1
	*/
	int this_function_mode, this_function_color;
	tie(this_function_mode, this_function_color) = get_function_status(slice_type);

	if (key_type == 1) {
		if (this_function_mode == 2) { // brush
			radius = radius + (float)dir;
			cgip_brush->setRadius(radius);
			printf("radius: %f\n", radius);
		}
		else if (this_function_mode == 5) { // magic brush
			if (cgip_magic_brush) {
				float r_mb = cgip_magic_brush->getRadius();
				r_mb = r_mb + (float)dir;
				cgip_magic_brush->setRadius(r_mb);
				printf("radius: %f\n", r_mb);
			}
		}
	}
	else if (key_type == 2) {
		if (action == 0) {
			action = 1;
		}
		else {
			action = 0;
		}

		if (this_function_mode == 1) {
			cgip_freedraw->setAction(action);
		}
		else if (this_function_mode == 2) {
			cgip_brush->setAction(action);
		}
		else if (this_function_mode == 3) {
			cgip_curve->setAction(action);
		}
		else if (this_function_mode == 4) {
			cgip_livewire->setAction(action);
		}
		else if (this_function_mode == 5) { // magic brush
			if (cgip_magic_brush) {
				float s = cgip_magic_brush->getSensitivity();
				s = s + (float)dir;
				cgip_magic_brush->setSensitivity(s);
				printf("sensitivity: %f\n", s);
			}
		}
	}
}
