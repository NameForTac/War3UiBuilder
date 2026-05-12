#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#include "elements/uielementdata.h"

class IniGenerator;

class ProjectManager : public QObject
{
    Q_OBJECT

public:
    explicit ProjectManager(QObject *parent = nullptr);
    ~ProjectManager() = default;

    void newProject();
    bool saveProject(const QString &path);
    bool loadProject(const QString &path);
    bool exportIni(const QString &path, bool war3Mode = false);

    QStringList importImages(const QStringList &sourcePaths, const QString &targetDir);

    void setElements(const QList<UiElementData> &elements) { m_elements = elements; }
    const QList<UiElementData> &elements() const { return m_elements; }

    QString projectDir() const { return m_projectDir; }

private:
    QList<UiElementData> m_elements;
    QString m_projectDir;
    QString m_projectFile;
    IniGenerator *m_iniGenerator;
};

#endif // PROJECTMANAGER_H
