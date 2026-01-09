#include "MainWindow.h"

#include <QDesktopServices>
#include <QEnterEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStatusBar>
#include <QTableView>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>


#include "Workspace.h" // core placeholder
#include "models/ContainerModel.h"
#include "models/ItemModel.h"

// Workaround for Qt 6.10.0 Windows Debug assert in QWindowsCursor (qpixmap_win.cpp:200)
// triggered by splitter resize cursor.
class SafeSplitterHandle : public QSplitterHandle {
public:
	using QSplitterHandle::QSplitterHandle;

protected:
	void enterEvent(QEnterEvent* e) override
	{
		QSplitterHandle::enterEvent(e);
		setCursor(Qt::ArrowCursor);
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

	containerModel_ = new ContainerModel(this);
	itemModel_ = new ItemModel(this);

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

void MainWindow::onItemSelectionChanged()
{
	detailsLabel_->clear();

	if (!itemsView_ || !itemProxy_)
		return;

	const QModelIndex proxyIndex = itemsView_->currentIndex();
	if (!proxyIndex.isValid())
		return;

	const QModelIndex srcIndex = itemProxy_->mapToSource(proxyIndex);
	if (!srcIndex.isValid())
		return;

	// Column 0 tooltip in ItemModel is absolute path (we set ToolTipRole for every column)
	const QString absPath = itemModel_->data(srcIndex, Qt::ToolTipRole).toString();
	const QString name = itemModel_->data(itemModel_->index(srcIndex.row(), 0), Qt::DisplayRole).toString();
	const QString size = itemModel_->data(itemModel_->index(srcIndex.row(), 1), Qt::DisplayRole).toString();
	const QString modified = itemModel_->data(itemModel_->index(srcIndex.row(), 2), Qt::DisplayRole).toString();

	detailsLabel_->setText(QString("Name: %1\nSize: %2\nModified: %3\nPath: %4")
		.arg(name, size, modified, absPath));
}

void MainWindow::onContainerSelectionChanged()
{
	const QModelIndex idx = containersView_->currentIndex();
	const QString containerPath = containerModel_->containerPathForIndex(idx);

	itemModel_->setDirectory(containerPath);

	if (filterEdit_)
		filterEdit_->clear();

	if (itemsView_)
		itemsView_->clearSelection();

	if (detailsLabel_)
		detailsLabel_->clear();
}

void MainWindow::onItemActivated(const QModelIndex& proxyIndex)
{
	if (!proxyIndex.isValid() || !itemProxy_)
		return;

	const QModelIndex srcIndex = itemProxy_->mapToSource(proxyIndex);
	if (!srcIndex.isValid())
		return;

	const QString absPath = itemModel_->data(srcIndex, Qt::ToolTipRole).toString();
	if (!absPath.isEmpty()) {
		QDesktopServices::openUrl(QUrl::fromLocalFile(absPath));
	}
}

void MainWindow::createCentralLayout()
{
	splitter_ = new SafeSplitter(this);
	splitter_->setOrientation(Qt::Horizontal);

	containersView_ = new QTreeView(splitter_);
	containersView_->setHeaderHidden(true);
	containersView_->setMinimumWidth(220);
	containersView_->setModel(containerModel_);

	// Right side container widget
	auto* rightWidget = new QWidget(splitter_);
	auto* rightLayout = new QVBoxLayout(rightWidget);
	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->setSpacing(6);

	// Filter row
	auto* filterRow = new QHBoxLayout();
	filterRow->setContentsMargins(0, 0, 0, 0);
	filterRow->setSpacing(6);

	auto* filterLabel = new QLabel("Filter:", rightWidget);
	filterEdit_ = new QLineEdit(rightWidget);
	filterEdit_->setPlaceholderText("type to filter by name...");

	filterRow->addWidget(filterLabel);
	filterRow->addWidget(filterEdit_, 1);

	rightLayout->addLayout(filterRow);

	// Proxy model for sorting/filtering
	itemProxy_ = new QSortFilterProxyModel(this);
	itemProxy_->setSourceModel(itemModel_);
	itemProxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
	itemProxy_->setFilterKeyColumn(0); // Name column
	itemProxy_->setSortCaseSensitivity(Qt::CaseInsensitive);

	itemsView_ = new QTableView(rightWidget);
	itemsView_->setModel(itemProxy_);
	itemsView_->setSortingEnabled(true);
	itemsView_->sortByColumn(0, Qt::AscendingOrder);
	itemsView_->horizontalHeader()->setStretchLastSection(true);
	itemsView_->setSelectionBehavior(QAbstractItemView::SelectRows);
	itemsView_->setSelectionMode(QAbstractItemView::ExtendedSelection);

	rightLayout->addWidget(itemsView_, 1);

	// Details pane
	detailsLabel_ = new QLabel(rightWidget);
	detailsLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
	detailsLabel_->setWordWrap(true);
	detailsLabel_->setMinimumHeight(44);
	rightLayout->addWidget(detailsLabel_);

	// Apply safe header cursor workaround you already used
	if (auto* hdr = itemsView_->horizontalHeader()) {
		hdr->setStretchLastSection(true);
		hdr->setCursor(Qt::ArrowCursor);
		hdr->viewport()->setCursor(Qt::ArrowCursor);
	}

	splitter_->addWidget(containersView_);
	splitter_->addWidget(rightWidget);

	// Workaround for Qt 6.10.0 Windows Debug cursor assert when hovering header dividers.
	if (auto* hdr = itemsView_->horizontalHeader()) {
		hdr->setSectionsMovable(false);
		hdr->setStretchLastSection(true);
		hdr->setCursor(Qt::ArrowCursor);
		hdr->viewport()->setCursor(Qt::ArrowCursor);
	}

	splitter_->setStretchFactor(0, 0);
	splitter_->setStretchFactor(1, 1);

	setCentralWidget(splitter_);

	// Update items when container selection changes
	connect(containersView_->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &MainWindow::onContainerSelectionChanged);

	connect(filterEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
		if (itemProxy_)
			itemProxy_->setFilterFixedString(text);
		});

	connect(itemsView_->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &MainWindow::onItemSelectionChanged);

	connect(itemsView_, &QTableView::doubleClicked,
		this, &MainWindow::onItemActivated);
}

void MainWindow::createStatusBar()
{
	statusBar()->showMessage(QString("Ready. CoreVersion=%1").arg(CoreVersion()));
}

void MainWindow::updateUiEnabled()
{
	if (actionOpenDir_)
		actionOpenDir_->setEnabled(true);
}

void MainWindow::openDirectory()
{
	const QString dir = QFileDialog::getExistingDirectory(
		this,
		"Open workspace directory"
	);

	if (dir.isEmpty())
		return;

	hasWorkspaceLoaded_ = true;

	containerModel_->setWorkspaceRoot(dir);

	// Select first container automatically
	if (containerModel_->rowCount() > 0) {
		const QModelIndex first = containerModel_->index(0, 0, {});
		containersView_->setCurrentIndex(first);
		containersView_->selectionModel()->select(first, QItemSelectionModel::ClearAndSelect);
		onContainerSelectionChanged();
	}

	statusBar()->showMessage(QString("Workspace: %1").arg(dir), 5000);
}
