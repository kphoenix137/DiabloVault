#include "MainWindow.h"

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <qfiledialog.h>

#include "Workspace.h" // from core/ (PUBLIC include dir via DiabloVault_core)

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("DiabloVault");
    resize(980, 640);

    createMenus();
    createCentralPlaceholder();
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

void MainWindow::createCentralPlaceholder()
{
    placeholder_ = new QLabel(this);
    placeholder_->setText(
        "DiabloVault\n\n"
        "File ? Open Directory…\n\n"
        "This will become the main workspace view."
    );
    placeholder_->setAlignment(Qt::AlignCenter);
    placeholder_->setWordWrap(true);

    setCentralWidget(placeholder_);
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
