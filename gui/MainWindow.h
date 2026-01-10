#pragma once

#include <QMainWindow>
#include <QString>

class QAction;
class QTreeView;
class QTableView;
class QSplitter;

class QLineEdit;
class QSortFilterProxyModel;
class QLabel;

class QModelIndex;

class ContainerModel;
class ItemModel;

namespace dv {
class Workspace;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openDirectory();
    void updateUiEnabled();
    void onContainerSelectionChanged();

    void onItemSelectionChanged();
    void onItemActivated(const QModelIndex& index);

private:
    void createMenus();
    void createCentralLayout();
    void createStatusBar();

    void openWorkspace(const QString& dir);
    void refreshWorkspace();

    // Actions
    QAction* actionOpenDir_ = nullptr;
    QAction* actionRefresh_ = nullptr;
    QAction* actionOpenInExplorer_ = nullptr;
    QAction* actionExit_ = nullptr;

    // UI
    QSplitter* splitter_ = nullptr;
    QTreeView* containersView_ = nullptr;
    QTableView* itemsView_ = nullptr;

    QSortFilterProxyModel* itemProxy_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QLabel* detailsLabel_ = nullptr;

    // Models
    ContainerModel* containerModel_ = nullptr;
    ItemModel* itemModel_ = nullptr;

    // Core
    dv::Workspace* workspace_ = nullptr;

    // State
    bool hasWorkspaceLoaded_ = false;
    QString currentWorkspaceDir_;
};
