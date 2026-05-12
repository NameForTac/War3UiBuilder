#include "projectmanager.h"
#include "export/inigenerator.h"
#include "elements/uielementdata.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent)
    , m_iniGenerator(new IniGenerator(this))
{
}

void ProjectManager::newProject()
{
    m_elements.clear();
    m_projectDir.clear();
    m_projectFile.clear();
}

bool ProjectManager::saveProject(const QString &path)
{
    QJsonObject root;
    QJsonArray elementsArray;

    for (const auto &el : m_elements) {
        QJsonObject obj;
        obj["name"] = el.name;
        obj["type"] = el.type;
        obj["texture"] = el.texture;
        obj["hasTexture"] = el.hasTexture;
        obj["x"] = el.x;
        obj["y"] = el.y;
        obj["width"] = el.width;
        obj["height"] = el.height;
        obj["parent"] = el.parent;

        // Type-specific properties
        obj["normalTexture"] = el.normalTexture;
        obj["highlightTexture"] = el.highlightTexture;
        obj["modelPath"] = el.modelPath;
        obj["textContent"] = el.textContent;
        obj["fontSize"] = el.fontSize;
        obj["textColor"] = el.textColor;

        QJsonArray children;
        for (const auto &child : el.children) {
            children.append(child);
        }
        obj["children"] = children;

        QJsonObject props;
        for (auto it = el.properties.begin(); it != el.properties.end(); ++it) {
            props[it.key()] = it.value();
        }
        obj["properties"] = props;

        elementsArray.append(obj);
    }

    root["elements"] = elementsArray;
    root["version"] = "1.0";

    QJsonDocument doc(root);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to save project:" << path;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    m_projectFile = path;
    m_projectDir = QFileInfo(path).absolutePath();

    return true;
}

bool ProjectManager::loadProject(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load project:" << path;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();
    m_elements.clear();

    QJsonArray elementsArray = root["elements"].toArray();
    for (const auto &val : elementsArray) {
        QJsonObject obj = val.toObject();

        UiElementData el;
        el.name = obj["name"].toString();
        el.type = obj["type"].toString("SIMPLEFRAME");
        el.texture = obj["texture"].toString();
        el.hasTexture = obj["hasTexture"].toBool(false);
        el.x = obj["x"].toDouble(0);
        el.y = obj["y"].toDouble(0);
        el.width = obj["width"].toDouble(100);
        el.height = obj["height"].toDouble(100);
        el.parent = obj["parent"].toString();

        // Type-specific properties
        el.normalTexture = obj["normalTexture"].toString();
        el.highlightTexture = obj["highlightTexture"].toString();
        el.modelPath = obj["modelPath"].toString();
        el.textContent = obj["textContent"].toString();
        el.fontSize = obj["fontSize"].toDouble(14.0);
        el.textColor = obj["textColor"].toString("#FFFFFF");

        QJsonArray children = obj["children"].toArray();
        for (const auto &child : children) {
            el.children.append(child.toString());
        }

        QJsonObject props = obj["properties"].toObject();
        for (auto it = props.begin(); it != props.end(); ++it) {
            el.properties[it.key()] = it.value().toString();
        }

        m_elements.append(el);
    }

    m_projectFile = path;
    m_projectDir = QFileInfo(path).absolutePath();

    return true;
}

bool ProjectManager::exportIni(const QString &path, bool war3Mode)
{
    return m_iniGenerator->generate(m_elements, m_projectDir, path, war3Mode);
}

QStringList ProjectManager::importImages(const QStringList &sourcePaths, const QString &targetDir)
{
    QStringList imported;

    QDir dir(targetDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Create a textures subdirectory
    QString texturesDir = targetDir + "/textures";
    QDir texDir(texturesDir);
    if (!texDir.exists()) {
        texDir.mkpath(".");
    }

    for (const QString &sourcePath : sourcePaths) {
        QFileInfo fi(sourcePath);
        QString targetPath = texturesDir + "/" + fi.fileName();

        if (QFile::exists(targetPath)) {
            // Remove existing file before copy
            QFile::remove(targetPath);
        }

        if (QFile::copy(sourcePath, targetPath)) {
            imported.append(targetPath);
        }
    }

    m_projectDir = targetDir;
    return imported;
}
