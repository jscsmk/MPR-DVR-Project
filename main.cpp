#include "mainwindow.h"
#include "glwidget.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QSurfaceFormat>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStatusBar>


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QCoreApplication::setApplicationName("My 3d Image Viewer");
	QCoreApplication::setOrganizationName("QtProject");
	QCoreApplication::setApplicationVersion(QT_VERSION_STR);
	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::applicationName());
	parser.addHelpOption();
	parser.addVersionOption();
	QCommandLineOption multipleSampleOption("multisample", "Multisampling");
	parser.addOption(multipleSampleOption);
	QCommandLineOption coreProfileOption("coreprofile", "Use core profile");
	parser.addOption(coreProfileOption);
	QCommandLineOption transparentOption("transparent", "Transparent window");
	parser.addOption(transparentOption);

	parser.process(app);

	QSurfaceFormat fmt;
	fmt.setDepthBufferSize(24);
	if (parser.isSet(multipleSampleOption))
		fmt.setSamples(4);
	if (parser.isSet(coreProfileOption)) {
		fmt.setVersion(3, 2);
		fmt.setProfile(QSurfaceFormat::CoreProfile);
	}
	QSurfaceFormat::setDefaultFormat(fmt);

	MainWindow mainWindow;
	mainWindow.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	//(mainWindow.statusBar())->setSizeGripEnabled(false);
	//mainWindow.setFixedSize(1300,800);

	GLWidget::setTransparent(parser.isSet(transparentOption));
	if (GLWidget::isTransparent()) {
		mainWindow.setAttribute(Qt::WA_TranslucentBackground);
		mainWindow.setAttribute(Qt::WA_NoSystemBackground, false);
	}
	mainWindow.resize(mainWindow.sizeHint());
	int desktopArea = QApplication::desktop()->width() *
		QApplication::desktop()->height();
	int widgetArea = mainWindow.width() * mainWindow.height();
	if (((float)widgetArea / (float)desktopArea) < 0.75f)
		mainWindow.show();
	else
		mainWindow.showMaximized();

	return app.exec();
}