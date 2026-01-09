#include "MainWindow.h"

#include "Workspace.h" // from core/ (PUBLIC include dir via DiabloVault_core)
#include <QEnterEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QSplitterHandle>
#include <QStatusBar>
#include <QTableView>
#include <QTreeView>
#include <qfiledialog.h>
#include <qsplitter.h>

class SafeSplitterHandle : public QSplitterHandle {
public:
	using QSplitterHandle::QSplitterHandle;

protected:
	void enterEvent(QEnterEvent* e) override
	{
		QSplitterHandle::enterEvent(e);
		setCursor(Qt::ArrowCursor); // avoid Qt's split cursor creation path
	}
};

class SafeSplitter : public QSplitter {
public:
	using QSplitter::QSplitter;

protected:
	QSplitterHandle* createHandle() override
	{
		auto* h = new SafeSplitterHandle(orientation(), this);
		h->setCursor(Qt::ArrowCursor);
		return h;
	}
};


MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	setWindowTitle("DiabloVault");
	resize(980, 640);

	createMenus();
	createCentralLayout();
	createStatusBar();

	updateUiEnabled();
}

void MainWindow::createMenus()
{
	auto* fileMenu = menuBar()->addMenu("&File");

	actionOpenDir_ = new QAction("Open &Directory...", this);
	actionOpenDir_->setShortcut(QKeySequence::Open);
	connect(actionOpenDir_, &QAction::triggered, this, &MainWindow::openDirectory);
	fileMenu->addAction(actionOpenDir_);

	fileMenu->addSeparator();

	actionExit_ = new QAction("E&xit", this);
	actionExit_->setShortcut(QKeySequence::Quit);
	connect(actionExit_, &QAction::triggered, this, &QWidget::close);
	fileMenu->addAction(actionExit_);
}

void MainWindow::createCentralLayout()
{
	// Workaround for Qt 6.10.0 Windows Debug assert in QWindowsCursor
	// (qpixmap_win.cpp:200) triggered by splitter resize cursor.
	splitter_ = new SafeSplitter(this);
	splitter_->setOrientation(Qt::Horizontal);

	containersView_ = new QTreeView(splitter_);
	containersView_->setHeaderHidden(true);
	containersView_->setMinimumWidth(220);

	itemsView_ = new QTableView(splitter_);
	itemsView_->horizontalHeader()->setStretchLastSection(true);
	itemsView_->setSelectionBehavior(QAbstractItemView::SelectRows);
	itemsView_->setSelectionMode(QAbstractItemView::ExtendedSelection);

	splitter_->addWidget(containersView_);
	splitter_->addWidget(itemsView_);
	splitter_->setStretchFactor(0, 0);
	splitter_->setStretchFactor(1, 1);

	setCentralWidget(splitter_);
}

void MainWindow::createStatusBar()
{
	// Show something “core is linked” without being a permanent UI feature.
	statusBar()->showMessage(QString("Ready. CoreVersion=%1").arg(CoreVersion()));
}


void MainWindow::updateUiEnabled()
{
	// For now, Open Directory is enabled always (since we don't load yet).
	// Later you might disable actions depending on workspace state.
	if (actionOpenDir_ != nullptr)
		actionOpenDir_->setEnabled(true);
}

void MainWindow::openDirectory()
{
	const QString dir = QFileDialog::getExistingDirectory(
		this,
		"Open Diablo save directory"
	);

	if (dir.isEmpty())
		return;

	hasWorkspaceLoaded_ = true;
	statusBar()->showMessage(QString("Selected: %1").arg(dir), 5000);
}
