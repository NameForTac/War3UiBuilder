#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>

#include "elements/uielementdata.h"

class UiView;
class UiScene;
class TreePanel;
class PropertyPanel;
class ProjectManager;
class UndoManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onExportIni();
    void onImportImage();
    void onDeleteSelected();
    void onCopy();
    void onCut();
    void onPaste();
    void onGridSnap();
    void onUndo();
    void onRedo();
    void onAbout();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupDockPanels();
    void setupCentralWidget();
    void connectSignals();

    void refreshPropertyPanel(const QString &name);

    UiScene *m_scene;
    UiView *m_view;
    TreePanel *m_treePanel;
    PropertyPanel *m_propertyPanel;
    ProjectManager *m_projectManager;
    UndoManager *m_undoManager;
    QString m_selectedElementName;
    QList<UiElementData> m_clipboard;

    void pushUndoState();
};

#endif // MAINWINDOW_H
