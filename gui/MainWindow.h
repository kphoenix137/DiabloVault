#pragma once

#include <QMainWindow>

class QAction;
class QLabel;
class QTreeView;
class QTableView;
class QSplitter;


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openDirectory();
    void updateUiEnabled();

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

    // State
    bool hasWorkspaceLoaded_ = false;
};
