#pragma once

#include <QMainWindow>

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

    // Actions
    QAction* actionOpenDir_ = nullptr;
    QAction* actionExit_ = nullptr;

    QSplitter* splitter_ = nullptr;
    QTreeView* containersView_ = nullptr;
    QTableView* itemsView_ = nullptr;

    QSortFilterProxyModel* itemProxy_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QLabel* detailsLabel_ = nullptr;

    ContainerModel* containerModel_ = nullptr;
    ItemModel* itemModel_ = nullptr;

    bool hasWorkspaceLoaded_ = false;
};
