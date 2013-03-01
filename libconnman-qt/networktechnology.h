/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef NETWORKTECHNOLOGY_H
#define NETWORKTECHNOLOGY_H

#include <QtDBus>

class Technology;

class NetworkTechnology : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY poweredChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString path READ path WRITE setPath)

public:
    NetworkTechnology(const QString &path, const QVariantMap &properties, QObject* parent);
    NetworkTechnology(QObject* parent=0);

    virtual ~NetworkTechnology();

    const QString name() const;
    const QString type() const;
    bool powered() const;
    bool connected() const;
    const QString objPath() const;

    QString path() const;

public slots:
    void setPowered(const bool &powered);
    void scan();
    void setPath(const QString &path);

signals:
    void poweredChanged(const bool &powered);
    void connectedChanged(const bool &connected);
    void scanFinished();

private:
    Technology *m_technology;
    QVariantMap m_propertiesCache;
    QDBusPendingCallWatcher *m_scanWatcher;

    static const QString Name;
    static const QString Type;
    static const QString Powered;
    static const QString Connected;

    QString m_path;
    void init(const QString &path);


private slots:
    void propertyChanged(const QString &name, const QDBusVariant &value);
    void scanReply(QDBusPendingCallWatcher *call);

private:
    Q_DISABLE_COPY(NetworkTechnology)
};

#endif //NETWORKTECHNOLOGY_H
