#pragma once

#include <QMainWindow>

class QAction;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openDirectory();
    void updateUiEnabled();

private:
    void createMenus();
    void createCentralPlaceholder();
    void createStatusBar();

    // Actions
    QAction* actionOpenDir_ = nullptr;
    QAction* actionExit_ = nullptr;

    // Simple placeholder content
    QLabel* placeholder_ = nullptr;

    // State
    bool hasWorkspaceLoaded_ = false;
};
