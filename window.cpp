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

#include "gdcmImageReader.h"
#include "gdcmReader.h"
#include "gdcmTag.h"
#include "gdcmFile.h"
#include "gdcmAttribute.h"
#include "sliceWidget.h"


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
	view_size = 600;
	slice_widget_z = new SliceWidget(0, view_size);
	slice_widget_x = new SliceWidget(1, view_size);
	slice_widget_y = new SliceWidget(2, view_size);
	slice_widget_z->setMouseTracking(true);
	slice_widget_x->setMouseTracking(true);
	slice_widget_y->setMouseTracking(true);
	slice_widget_z->setContentsMargins(0, 0, 0, 20);
	slice_widget_x->setContentsMargins(0, 0, 0, 20);
	data_cube = new DataCube();
	data_3d = NULL;
	dvr_widget = NULL;

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
	QAction *toggle_slice_line_z, *toggle_slice_line_x, *toggle_slice_line_y;
	QAction *toggle_border_line_z, *toggle_border_line_x, *toggle_border_line_y;
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

	add_menubar_button(menubar_layout_z, toggle_menu_z, "icons/view.png");
	add_menubar_button(menubar_layout_z, init_menu_z, "icons/reset.png");
	add_menubar_button(menubar_layout_x, toggle_menu_x, "icons/view.png");
	add_menubar_button(menubar_layout_x, init_menu_x, "icons/reset.png");
	add_menubar_button(menubar_layout_y, toggle_menu_y, "icons/view.png");
	add_menubar_button(menubar_layout_y, init_menu_y, "icons/reset.png");
	add_menubar_button(menubar_layout_dvr, toggle_menu_dvr, "icons/view.png");
	add_menubar_button(menubar_layout_dvr, init_menu_dvr, "icons/reset.png");

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

	coord_z = create_label(slice_widget_z);
	coord_x = create_label(slice_widget_x);
	coord_y = create_label(slice_widget_y);
	window_z = create_label(slice_widget_z);
	window_x = create_label(slice_widget_x);
	window_y = create_label(slice_widget_y);

	coord_z->move(view_size * 7 / 4 - 150, view_size - 40);
	coord_x->move(view_size * 7 / 4 - 150, view_size - 40);
	coord_y->move(view_size * 7 / 4 - 150, view_size - 40);



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

	connect(slice_widget_z, SIGNAL(coord_info_sig(QString)), coord_z, SLOT(setText(QString)));
	connect(slice_widget_x, SIGNAL(coord_info_sig(QString)), coord_x, SLOT(setText(QString)));
	connect(slice_widget_y, SIGNAL(coord_info_sig(QString)), coord_y, SLOT(setText(QString)));

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
	*blank_img = blank_img->scaled(view_size * 7 / 4, view_size);

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
	data_3d = (int*)malloc(z * x * y * sizeof(int));

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
			for (int j = 0; j < y; j++)
			{
				for (int k = 0; k < x; k++)
				{
					short stored_pixel_value = *buffer16;
					data_3d[x*y*i + x * j + k] = (int)stored_pixel_value;
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


	int slice_pixel_num = 512;
	int dvr_pixel_num = 512;
	float unit_ray_len = 1;
	skipping_mode = true;
	QString skip_text = "empty-space skipping: ON";

	data_cube->set_data(data_3d, x, y, z, slice_pixel_num, a, b, slice_thickness, p_min, p_max);

	slice_widget_z->set_data(data_cube);
	slice_widget_x->set_data(data_cube);
	slice_widget_y->set_data(data_cube);

	if (!dvr_widget) {
		container_3->removeItem(container_3->itemAt(3));
		blank_dvr->setVisible(false);

		dvr_widget = new DVRWidget(data_cube, unit_ray_len, a, b, dvr_pixel_num, view_size);
		//dvr_widget->set_data(data_cube, dvr_pixel_num, unit_ray_len, a, b);
		dvr_widget->setMouseTracking(true);

		window_dvr = create_label(dvr_widget);
		//coord_dvr = create_label(dvr_widget);
		//coord_dvr->move(2 * view_size - 260, view_size - 40);
		skipping_label = create_label(dvr_widget);
		skipping_label->move(10, 10);
		skipping_label->setText(skip_text);

		//connect(dvr_widget, SIGNAL(coord_info_sig(QString)), coord_dvr, SLOT(setText(QString)));
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
		window_dvr->move(10, view_size - 40);
		skipping_label->setParent(dvr_widget);
		skipping_label->move(10, 10);
		skipping_label->setText(skip_text);
		//coord_dvr->setParent(dvr_widget);
		//coord_dvr->move(2 * view_size - 260, view_size - 40);
	}
}

QLabel *Window::create_label(QWidget *sw)
{
	QLabel *label = new QLabel;
	label->setFixedSize(180, 30);
	label->setParent(sw);
	label->move(10, view_size - 40);

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

void Window::add_menubar_button(QHBoxLayout *m_layout, QMenu *m, QString icon_path)
{
	QPushButton *new_button = new QPushButton;
	new_button->setStyleSheet("text-align:left;");
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

void Window::update_dvr_slices()
{
	if (dvr_widget)
		dvr_widget->get_slice_info();
}

void Window::toggle_skipping_label()
{
	QString skip_text;
	skip_text = skipping_mode ? "empty-space skipping: OFF" : "empty-space skipping: ON";
	skipping_label->setText(skip_text);
	skipping_mode = skipping_mode ? false : true;
}


void Window::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
		close();
	else
		QWidget::keyPressEvent(e);
}


/*
void Window::onMapped_1(QWidget *widget)
{
	QSlider *slider = qobject_cast<QSlider*>(signalMapper_1->mapping(widget));
	QLabel *label = qobject_cast<QLabel*>(widget);
	idx_z = slider->value();
	set_pixmap(1);
}
void Window::onMapped_2(QWidget *widget)
{
	QSlider *slider = qobject_cast<QSlider*>(signalMapper_2->mapping(widget));
	QLabel *label = qobject_cast<QLabel*>(widget);
	idx_x = slider->value();
	set_pixmap(1);
}
void Window::onMapped_3(QWidget *widget)
{
	QSlider *slider = qobject_cast<QSlider*>(signalMapper_3->mapping(widget));
	QLabel *label = qobject_cast<QLabel*>(widget);
	idx_y = slider->value();
	set_pixmap(1);
}

QSlider *Window::createSlider(int x)
{
	QSlider *slider;
	if (x == 0)
		slider = new QSlider(Qt::Vertical, 0);
	else
		slider = new QSlider(Qt::Horizontal, 0);

	slider->setRange(0, 360 * 16);
	slider->setSingleStep(16);
	slider->setPageStep(15 * 16);
	slider->setTickInterval(15 * 16);
	slider->setTickPosition(QSlider::TicksRight);
	return slider;
}

void Window::dockUndock()
{
	if (parent()) {
		setParent(0);
		setAttribute(Qt::WA_DeleteOnClose);
		move(QApplication::desktop()->width() / 2 - width() / 2,
			QApplication::desktop()->height() / 2 - height() / 2);
		dockBtn->setText(tr("Dock"));
		show();
	}
	else {
		if (!mainWindow->centralWidget()) {
			if (mainWindow->isVisible()) {
				setAttribute(Qt::WA_DeleteOnClose, false);
				dockBtn->setText(tr("Undock"));
				mainWindow->setCentralWidget(this);
			}
			else {
				QMessageBox::information(0, tr("Cannot dock"), tr("Main window already closed"));
			}
		}
		else {
			QMessageBox::information(0, tr("Cannot dock"), tr("Main window already occupied"));
		}
	}
}*/
