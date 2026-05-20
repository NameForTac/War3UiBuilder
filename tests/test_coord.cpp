#include <QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>

#include "uielementdata.h"
#include "inigenerator.h"
#include "fdfgenerator.h"
#include "undomanager.h"

// ── Coordinate conversion (mirrors IniGenerator logic) ──
static double war3X(double relPx)  { return relPx / 1920.0 * 0.8; }
static double war3Y(double relPx)  { return -relPx / 1080.0 * 0.6; }
static double war3W(double pw)     { return pw / 1920.0 * 0.8; }
static double war3H(double ph)     { return ph / 1080.0 * 0.6; }

// ── Cycle detection (mirrors UiScene::wouldCreateCycle) ──
static bool wouldCreateCycle(const QMap<QString, QString> &parents,
                             const QString &childName,
                             const QString &potentialParent)
{
    if (childName == potentialParent) return true;
    QString current = potentialParent;
    while (!current.isEmpty()) {
        if (current == childName) return true;
        if (!parents.contains(current)) break;
        current = parents[current];
    }
    return false;
}

// Helper to create test elements
static UiElementData makeElement(const QString &name, const QString &type = "SIMPLEFRAME",
                                  double x = 0, double y = 0,
                                  double w = 100, double h = 100,
                                  const QString &parent = QString())
{
    UiElementData el;
    el.name = name;
    el.type = type;
    el.x = x;
    el.y = y;
    el.width = w;
    el.height = h;
    el.parent = parent;
    return el;
}

class TestWar3UiBuilder : public QObject
{
    Q_OBJECT

private slots:
    // ── Coord Tests ──
    void testWar3X()
    {
        QCOMPARE(war3X(0.0), 0.0);
        QCOMPARE(war3X(960.0), 0.4);
        QCOMPARE(war3X(1920.0), 0.8);
    }

    void testWar3Y()
    {
        QCOMPARE(war3Y(0.0), 0.0);
        QCOMPARE(war3Y(100.0), -100.0 / 1080.0 * 0.6);
        QCOMPARE(war3Y(-100.0), 100.0 / 1080.0 * 0.6);
    }

    void testWar3WH()
    {
        QCOMPARE(war3W(100.0), 100.0 / 1920.0 * 0.8);
        QCOMPARE(war3H(100.0), 100.0 / 1080.0 * 0.6);
        QCOMPARE(war3W(1920.0), 0.8);
        QCOMPARE(war3H(1080.0), 0.6);
    }

    // ── Serialization Tests ──
    void testRoundTrip()
    {
        UiElementData el;
        el.name = "TestButton";
        el.type = "BUTTON";
        el.x = 100.0; el.y = 200.0;
        el.width = 300.0; el.height = 50.0;
        el.parent = "Root";
        el.hasTexture = true;
        el.texture = "textures/btn.dds";
        el.textContent = "Click Me";
        el.fontSize = 16.0;
        el.textColor = "#FF0000";

        QJsonObject obj;
        obj["name"] = el.name;
        obj["type"] = el.type;
        obj["x"] = el.x; obj["y"] = el.y;
        obj["width"] = el.width; obj["height"] = el.height;
        obj["parent"] = el.parent;
        obj["hasTexture"] = el.hasTexture;
        obj["texture"] = el.texture;
        obj["textContent"] = el.textContent;
        obj["fontSize"] = el.fontSize;
        obj["textColor"] = el.textColor;

        UiElementData el2;
        el2.name = obj["name"].toString();
        el2.type = obj["type"].toString();
        el2.x = obj["x"].toDouble();
        el2.y = obj["y"].toDouble();
        el2.width = obj["width"].toDouble();
        el2.height = obj["height"].toDouble();
        el2.parent = obj["parent"].toString();
        el2.hasTexture = obj["hasTexture"].toBool();
        el2.texture = obj["texture"].toString();
        el2.textContent = obj["textContent"].toString();
        el2.fontSize = obj["fontSize"].toDouble();
        el2.textColor = obj["textColor"].toString();

        QCOMPARE(el2.name, el.name);
        QCOMPARE(el2.type, el.type);
        QCOMPARE(el2.x, el.x);
        QCOMPARE(el2.y, el.y);
        QCOMPARE(el2.parent, el.parent);
        QCOMPARE(el2.textContent, el.textContent);
    }

    void testIsValid()
    {
        UiElementData empty;
        QVERIFY(!empty.isValid());

        UiElementData valid;
        valid.name = "Foo";
        QVERIFY(valid.isValid());
    }

    // ── Extended Serialization: full project JSON ──
    void testFullProjectSerialization()
    {
        // Simulate what ProjectManager does: serialize a list of elements to/from JSON
        QList<UiElementData> elements;
        elements.append(makeElement("Root", "SIMPLEFRAME", 0, 0, 1920, 1080));
        elements.append(makeElement("Btn", "BUTTON", 100, 200, 200, 50, "Root"));
        elements.append(makeElement("Lbl", "TEXT", 50, 50, 100, 30, "Root"));

        // Add type-specific properties to Btn
        auto &btn = elements[1];
        btn.normalTexture = "textures/btn_n.dds";
        btn.highlightTexture = "textures/btn_h.dds";
        btn.properties["CustomKey"] = "CustomValue";

        // Serialize to JSON (same as ProjectManager::saveProject)
        QJsonArray arr;
        for (const auto &el : elements) {
            QJsonObject obj;
            obj["name"] = el.name;
            obj["type"] = el.type;
            obj["x"] = el.x;
            obj["y"] = el.y;
            obj["width"] = el.width;
            obj["height"] = el.height;
            obj["parent"] = el.parent;
            obj["hasTexture"] = el.hasTexture;
            obj["texture"] = el.texture;
            obj["normalTexture"] = el.normalTexture;
            obj["highlightTexture"] = el.highlightTexture;
            obj["textContent"] = el.textContent;
            obj["fontSize"] = el.fontSize;
            obj["textColor"] = el.textColor;
            QJsonObject props;
            for (auto it = el.properties.begin(); it != el.properties.end(); ++it)
                props[it.key()] = it.value();
            obj["properties"] = props;
            arr.append(obj);
        }
        QJsonObject root;
        root["elements"] = arr;
        root["version"] = "1.0";

        // Deserialize (same as ProjectManager::loadProject)
        QList<UiElementData> loaded;
        QJsonArray loadedArr = root["elements"].toArray();
        for (const auto &val : loadedArr) {
            QJsonObject obj = val.toObject();
            UiElementData el;
            el.name = obj["name"].toString();
            el.type = obj["type"].toString("SIMPLEFRAME");
            el.x = obj["x"].toDouble(0);
            el.y = obj["y"].toDouble(0);
            el.width = obj["width"].toDouble(100);
            el.height = obj["height"].toDouble(100);
            el.parent = obj["parent"].toString();
            el.hasTexture = obj["hasTexture"].toBool(false);
            el.texture = obj["texture"].toString();
            el.normalTexture = obj["normalTexture"].toString();
            el.highlightTexture = obj["highlightTexture"].toString();
            el.textContent = obj["textContent"].toString();
            el.fontSize = obj["fontSize"].toDouble(14.0);
            el.textColor = obj["textColor"].toString("#FFFFFF");
            QJsonObject props = obj["properties"].toObject();
            for (auto it = props.begin(); it != props.end(); ++it)
                el.properties[it.key()] = it.value().toString();
            loaded.append(el);
        }

        QCOMPARE(loaded.size(), 3);
        QCOMPARE(loaded[0].name, "Root");
        QCOMPARE(loaded[1].type, "BUTTON");
        QCOMPARE(loaded[1].parent, "Root");
        QCOMPARE(loaded[1].normalTexture, "textures/btn_n.dds");
        QCOMPARE(loaded[1].highlightTexture, "textures/btn_h.dds");
        QCOMPARE(loaded[1].properties["CustomKey"], "CustomValue");
        QCOMPARE(loaded[2].type, "TEXT");
    }

    void testVersionField()
    {
        QJsonObject root;
        root["version"] = "1.0";
        root["elements"] = QJsonArray();
        QCOMPARE(root["version"].toString(), "1.0");
    }

    // ── INI Export Tests ──
    void testIniExportBasic()
    {
        IniGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("test.ini");

        QList<UiElementData> elements;
        elements.append(makeElement("Root", "SIMPLEFRAME", 0, 0, 1920, 1080));
        elements.append(makeElement("Child", "BACKDROP", 100, 200, 300, 150, "Root"));

        QVERIFY(gen.generate(elements, tmpDir.path(), outputPath, false));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains("[UI]"));
        QVERIFY(content.contains("Frame_Root_Type = SIMPLEFRAME"));
        QVERIFY(content.contains("Frame_Child_Parent = Root"));
        QVERIFY(content.contains("Frame_Child_X = 100.0"));
        QVERIFY(content.contains("Frame_Child_Y = 200.0"));
        QVERIFY(content.contains("Frame_Child_Width = 300.0"));
        QVERIFY(content.contains("Frame_Child_Height = 150.0"));
    }

    void testIniExportWar3Mode()
    {
        IniGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("test_war3.ini");

        QList<UiElementData> elements;
        elements.append(makeElement("Root", "SIMPLEFRAME", 0, 0, 1920, 1080));
        elements.append(makeElement("Child", "FRAME", 960, 540, 100, 50, "Root"));

        QVERIFY(gen.generate(elements, tmpDir.path(), outputPath, true));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        // Child relative to Root: X=960 => 0.4, Y=540 => -0.3
        QVERIFY(content.contains("Frame_Child_X = 0.4000"));
        QVERIFY(content.contains("Frame_Child_Y = -0.3000"));
        QVERIFY(content.contains("Frame_Child_Width = 0.0417"));
        QVERIFY(content.contains("Frame_Child_Height = 0.0278"));
    }

    void testIniExportWithTexture()
    {
        IniGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("tex.ini");

        QList<UiElementData> elements;
        auto el = makeElement("Back", "BACKDROP", 0, 0, 500, 500);
        el.hasTexture = true;
        el.texture = "textures/my_tex.dds";
        elements.append(el);

        QVERIFY(gen.generate(elements, tmpDir.path(), outputPath, false));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains("Frame_Back_Texture = war3mapUI\\textures\\my_tex.dds"));
    }

    void testIniExportButton()
    {
        IniGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("btn.ini");

        QList<UiElementData> elements;
        auto el = makeElement("Btn", "BUTTON", 0, 0, 100, 50);
        el.normalTexture = "textures/normal.dds";
        el.highlightTexture = "textures/highlight.dds";
        elements.append(el);

        QVERIFY(gen.generate(elements, tmpDir.path(), outputPath, false));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains("Frame_Btn_Type = BUTTON"));
        QVERIFY(content.contains("Frame_Btn_NormalTexture = war3mapUI\\textures\\normal.dds"));
        QVERIFY(content.contains("Frame_Btn_HighlightTexture = war3mapUI\\textures\\highlight.dds"));
    }

    void testIniExportText()
    {
        IniGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("txt.ini");

        QList<UiElementData> elements;
        auto el = makeElement("Txt", "TEXT", 0, 0, 200, 30);
        el.textContent = "Hello";
        el.fontSize = 18.0;
        el.textColor = "#00FF00";
        elements.append(el);

        QVERIFY(gen.generate(elements, tmpDir.path(), outputPath, false));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains("Frame_Txt_Text = Hello"));
        QVERIFY(content.contains("Frame_Txt_FontSize = 18"));
        QVERIFY(content.contains("Frame_Txt_TextColor = #00FF00"));
    }

    void testIniExportEmpty()
    {
        IniGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("empty.ini");

        QList<UiElementData> empty;
        QVERIFY(gen.generate(empty, tmpDir.path(), outputPath, false));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QCOMPARE(content, "[UI]\n");
    }

    // ── FDF Export Tests ──
    void testFdfExportBasic()
    {
        FdfGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("test.fdf");

        QList<UiElementData> elements;
        elements.append(makeElement("Root", "SIMPLEFRAME", 0, 0, 1920, 1080));
        elements.append(makeElement("Child", "BACKDROP", 100, 200, 300, 150, "Root"));

        QVERIFY(gen.exportFdf(elements, outputPath, false));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        // Check FDF structure
        QVERIFY(content.contains("Include \"UI\\\\FrameDef\\\\UI\\\\FrameDef.toc\""));
        QVERIFY(content.contains("Frame \"Root\" {"));
        QVERIFY(content.contains("Frame \"Child\" {"));
        QVERIFY(content.contains("Type \"SIMPLEFRAME\""));
        QVERIFY(content.contains("Type \"BACKDROP\""));
        // Child should be nested inside Root
        QVERIFY(content.indexOf("Frame \"Root\" {") < content.indexOf("Frame \"Child\" {"));
    }

    void testFdfExportWar3Mode()
    {
        FdfGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("test_war3.fdf");

        QList<UiElementData> elements;
        elements.append(makeElement("Root", "SIMPLEFRAME", 0, 0, 1920, 1080));
        elements.append(makeElement("Child", "FRAME", 960, 540, 100, 50, "Root"));

        QVERIFY(gen.exportFdf(elements, outputPath, true));

        QFile file(outputPath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QVERIFY(content.contains("X 0.4000"));
        QVERIFY(content.contains("Y -0.3000"));
    }

    // ── FDF Import/Export Round-Trip Tests ──
    void testFdfRoundTrip()
    {
        FdfGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("roundtrip.fdf");

        QList<UiElementData> original;
        original.append(makeElement("Root", "SIMPLEFRAME", 0, 0, 1920, 1080));
        original.append(makeElement("Child", "BACKDROP", 100, 200, 300, 150, "Root"));

        QVERIFY(gen.exportFdf(original, outputPath, false));

        // Import back
        QString errorMsg;
        QList<UiElementData> imported = gen.importFdf(outputPath, &errorMsg);
        QVERIFY2(!imported.isEmpty(), errorMsg.toUtf8().constData());

        // Find elements by name
        auto findByName = [&](const QString &name) -> UiElementData {
            for (const auto &el : imported)
                if (el.name == name) return el;
            return UiElementData();
        };

        UiElementData root = findByName("Root");
        QVERIFY(root.isValid());
        QCOMPARE(root.type, "SIMPLEFRAME");
        QCOMPARE(qRound(root.width), 1920);
        QCOMPARE(qRound(root.height), 1080);

        UiElementData child = findByName("Child");
        QVERIFY(child.isValid());
        QCOMPARE(child.type, "BACKDROP");
        QCOMPARE(child.parent, "Root");
        // Because the child is parent-relative in FDF, import adjusts:
        // FDF exports with relX/Y, but import reads absolute since there's no parent context
        // Check that parent relationship is maintained
        QCOMPARE(child.parent, "Root");
    }

    void testFdfRoundTripComplex()
    {
        FdfGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString outputPath = tmpDir.filePath("complex.fdf");

        QList<UiElementData> original;
        original.append(makeElement("Root", "SIMPLEFRAME", 0, 0, 1920, 1080));
        original.append(makeElement("Btn", "BUTTON", 100, 200, 200, 50, "Root"));
        original.append(makeElement("Lbl", "TEXT", 50, 50, 100, 30, "Root"));

        // Add type-specific properties
        auto &btn = original[1];
        btn.normalTexture = "textures/normal.dds";
        btn.highlightTexture = "textures/highlight.dds";

        auto &lbl = original[2];
        lbl.textContent = "Hello!";
        lbl.fontSize = 16.0;
        lbl.textColor = "#FF8800";

        QVERIFY(gen.exportFdf(original, outputPath, false));

        QString errorMsg;
        QList<UiElementData> imported = gen.importFdf(outputPath, &errorMsg);
        QVERIFY2(!imported.isEmpty(), errorMsg.toUtf8().constData());

        QVERIFY(imported.size() >= 3);

        // Verify button re-import
        auto findByName = [&](const QString &name) -> UiElementData {
            for (const auto &el : imported)
                if (el.name == name) return el;
            return UiElementData();
        };

        UiElementData btnImported = findByName("Btn");
        QVERIFY(btnImported.isValid());
        QCOMPARE(btnImported.type, "BUTTON");
        QCOMPARE(btnImported.parent, "Root");

        UiElementData lblImported = findByName("Lbl");
        QVERIFY(lblImported.isValid());
        QCOMPARE(lblImported.type, "TEXT");
        QCOMPARE(lblImported.parent, "Root");
    }

    void testFdfImportWithComments()
    {
        FdfGenerator gen;
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString fdfPath = tmpDir.filePath("comments.fdf");

        // Write FDF with comments
        QFile file(fdfPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(
            "// This is a comment\n"
            "Include \"UI\\\\FrameDef\\\\UI\\\\FrameDef.toc\",\n"
            "/* Multi-line\n"
            "   comment */\n"
            "Frame \"TestFrame\" {\n"
            "    Type \"SIMPLEFRAME\",\n"
            "    Width 100.0,\n"
            "    Height 50.0,\n"
            "}\n"
        );
        file.close();

        QString errorMsg;
        QList<UiElementData> elements = gen.importFdf(fdfPath, &errorMsg);
        QVERIFY2(!elements.isEmpty(), errorMsg.toUtf8().constData());
        QCOMPARE(elements.size(), 1);
        QCOMPARE(elements[0].name, "TestFrame");
        QCOMPARE(elements[0].type, "SIMPLEFRAME");
        QCOMPARE(qRound(elements[0].width), 100);
        QCOMPARE(qRound(elements[0].height), 50);
    }

    // ── Undo Manager Tests ──
    void testUndoManagerBasic()
    {
        UndoManager um;
        QVERIFY(!um.canUndo());
        QVERIFY(!um.canRedo());

        QList<UiElementData> state1;
        state1.append(makeElement("A"));
        um.pushState(state1);
        QVERIFY(!um.canUndo());  // first push doesn't enable undo (it's current state)
        QVERIFY(!um.canRedo());

        QList<UiElementData> state2;
        state2.append(makeElement("A"));
        state2.append(makeElement("B"));
        um.pushState(state2);
        QVERIFY(um.canUndo());
        QVERIFY(!um.canRedo());
    }

    void testUndoRedo()
    {
        UndoManager um;

        // Initial state
        QList<UiElementData> initial;
        initial.append(makeElement("A"));
        um.pushState(initial);

        // Make change
        QList<UiElementData> changed;
        changed.append(makeElement("A"));
        changed.append(makeElement("B"));
        um.pushState(changed);

        // Undo
        QVERIFY(um.canUndo());
        auto undone = um.undo();
        QCOMPARE(undone.size(), 1);
        QVERIFY(um.canRedo());

        // Redo
        QVERIFY(um.canRedo());
        auto redone = um.redo();
        QCOMPARE(redone.size(), 2);
        QVERIFY(um.canUndo());
        QVERIFY(!um.canRedo());
    }

    void testUndoMultipleSteps()
    {
        UndoManager um;

        QList<UiElementData> s1; s1.append(makeElement("A"));
        QList<UiElementData> s2; s2.append(makeElement("A")); s2.append(makeElement("B"));
        QList<UiElementData> s3; s3.append(makeElement("A")); s3.append(makeElement("B")); s3.append(makeElement("C"));

        um.pushState(s1);
        um.pushState(s2);
        um.pushState(s3);

        auto step2 = um.undo();
        QCOMPARE(step2.size(), 2);

        auto step1 = um.undo();
        QCOMPARE(step1.size(), 1);

        // No more undo
        QVERIFY(!um.canUndo());

        // Redo back
        auto r2 = um.redo();
        QCOMPARE(r2.size(), 2);
        auto r3 = um.redo();
        QCOMPARE(r3.size(), 3);
    }

    void testUndoClear()
    {
        UndoManager um;
        QList<UiElementData> s1; s1.append(makeElement("A"));
        QList<UiElementData> s2; s2.append(makeElement("A")); s2.append(makeElement("B"));

        um.pushState(s1);
        um.pushState(s2);
        QVERIFY(um.canUndo());

        um.clear();
        QVERIFY(!um.canUndo());
        QVERIFY(!um.canRedo());
    }

    void testUndoAfterRedoBreak()
    {
        UndoManager um;

        QList<UiElementData> s1; s1.append(makeElement("A"));
        QList<UiElementData> s2; s2.append(makeElement("A")); s2.append(makeElement("B"));
        QList<UiElementData> s3; s3.append(makeElement("A")); s3.append(makeElement("B")); s3.append(makeElement("C"));
        QList<UiElementData> s4; s4.append(makeElement("X"));

        um.pushState(s1);
        um.pushState(s2);
        um.pushState(s3);

        um.undo(); // back to s2
        um.undo(); // back to s1
        um.redo(); // back to s2

        // Push new state — should invalidate s3 redo, stack is now [s1, s2, s4]
        um.pushState(s4);

        QVERIFY(um.canUndo());
        QVERIFY(!um.canRedo());

        auto undone = um.undo();
        QCOMPARE(undone.size(), 2); // back to s2 (previous state before s4)
    }

    // ── Cycle Detection Tests ──
    void testSelfParent()
    {
        QMap<QString, QString> parents{{"A", ""}};
        QVERIFY(wouldCreateCycle(parents, "A", "A"));
    }

    void testSimpleCycle()
    {
        QMap<QString, QString> parents{{"A", ""}, {"B", "A"}};
        QVERIFY(wouldCreateCycle(parents, "A", "B"));
    }

    void testNoCycle()
    {
        QMap<QString, QString> parents{{"A", ""}, {"B", ""}};
        QVERIFY(!wouldCreateCycle(parents, "A", "B"));
    }

    void testDeepNoCycle()
    {
        QMap<QString, QString> parents{{"A", ""}, {"B", "A"}, {"C", "B"}};
        QVERIFY(!wouldCreateCycle(parents, "C", "A"));
        QVERIFY(wouldCreateCycle(parents, "A", "C"));
    }

    void testUnknownParent()
    {
        QMap<QString, QString> parents{{"A", ""}};
        QVERIFY(!wouldCreateCycle(parents, "A", "UNKNOWN"));
    }
};

// Manual main — .moc must be generated by MOC and included here
int main(int argc, char *argv[])
{
    TestWar3UiBuilder test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_coord.moc"
