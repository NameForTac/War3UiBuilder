#ifndef INIGENERATOR_H
#define INIGENERATOR_H

#include <QObject>
#include <QString>
#include <QList>

struct UiElementData;

class IniGenerator : public QObject
{
    Q_OBJECT

public:
    explicit IniGenerator(QObject *parent = nullptr);
    ~IniGenerator() = default;

    bool generate(const QList<UiElementData> &elements,
                  const QString &projectDir,
                  const QString &outputPath,
                  bool war3Mode = false);

private:
    QString propertyLine(const QString &prefix, const QString &key, const QString &value) const;
    void writeElement(QString &output, const UiElementData &element,
                      const QMap<QString, UiElementData> &dataMap,
                      QSet<QString> &written,
                      bool war3Mode) const;
    static double toWar3X(double px, double parentPx = 0.0);
    static double toWar3Y(double py, double parentPy = 0.0);
    static QString makeTextureRef(const QString &texture);
};

#endif // INIGENERATOR_H
