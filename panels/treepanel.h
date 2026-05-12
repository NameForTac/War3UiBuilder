#ifndef TREEPANEL_H
#define TREEPANEL_H

#include <QWidget>
#include <QTreeWidget>
#include <QList>

struct UiElementData;

class TreePanel : public QWidget
{
    Q_OBJECT

public:
    explicit TreePanel(QWidget *parent = nullptr);
    ~TreePanel() = default;

    void rebuildTree(const QList<UiElementData> &elements);
    void selectElement(const QString &name);
    void refreshTree(const QList<UiElementData> &elements);
    void clearPanel();

signals:
    void elementSelected(const QString &name);
    void visibilityToggled(const QString &name, bool visible);
    void lockToggled(const QString &name, bool locked);
    void parentChanged(const QString &elementName, const QString &newParent);
    void renameRequested(const QString &oldName, const QString &newName);
    void deleteRequested(const QString &name);

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onCustomContextMenu(const QPoint &pos);
    void processPendingDrop();

private:
    QTreeWidgetItem *findItemByName(const QString &name) const;
    void buildTreeRecursive(QTreeWidgetItem *parent, const QString &parentName,
                            const QMap<QString, UiElementData> &dataMap,
                            QMap<QString, QTreeWidgetItem *> &itemMap);
    void setItemStatus(QTreeWidgetItem *item, bool visible, bool locked);
    void scanTreeState(QTreeWidgetItem *item, const QString &parentName,
                       QMap<QString, QString> &state) const;
    void saveTreeState();
    bool m_checkPending = false;
    QMap<QString, QString> m_lastTreeState;
    QTreeWidget *m_tree;
};

#endif // TREEPANEL_H
