/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "debug.h"
#include "useragent.h"
#include "networkmanager.h"

static const char AGENT_PATH[] = "/ConnectivityUserAgent";

UserAgent::UserAgent(QObject* parent) :
    QObject(parent),
    m_req_data(NULL),
    m_manager(NetworkManagerFactory::createInstance()),
    requestType(TYPE_DEFAULT)
{
    new AgentAdaptor(this); // this object will be freed when UserAgent is freed
    QDBusConnection::systemBus().registerObject(AGENT_PATH, this);

    if (m_manager->isAvailable()) {
        m_manager->unregisterAgent(QString(AGENT_PATH));
        m_manager->registerAgent(QString(AGENT_PATH));
    }
    connect(m_manager, SIGNAL(availabilityChanged(bool)),
            this, SLOT(updateMgrAvailability(bool)));
}

UserAgent::~UserAgent()
{
    m_manager->unregisterAgent(QString(AGENT_PATH));
}

void UserAgent::requestUserInput(ServiceRequestData* data)
{
    m_req_data = data;
    QVariantList fields;

    foreach (const QString &key, data->fields.keys()) {
        QVariantMap field;

        field.insert("name", key);
        field.insert("type", data->fields.value(key).toMap().value("Type"));
        fields.append(QVariant(field));
    }

    emit userInputRequested(data->objectPath, fields);
}

void UserAgent::cancelUserInput()
{
    delete m_req_data;
    m_req_data = NULL;
    emit userInputCanceled();
}

void UserAgent::reportError(const QString &error)
{
    emit errorReported(error);
}

void UserAgent::sendUserReply(const QVariantMap &input)
{
    if (m_req_data == NULL) {
        qWarning("Got reply for non-existing request");
        return;
    }

    if (!input.isEmpty()) {
        QDBusMessage &reply = m_req_data->reply;
        reply << input;
        QDBusConnection::systemBus().send(reply);
    } else {
        QDBusMessage error = m_req_data->msg.createErrorReply(
            QString("net.connman.Agent.Error.Canceled"),
            QString("canceled by user"));
        QDBusConnection::systemBus().send(error);
    }
    delete m_req_data;
    m_req_data = NULL;
}

void UserAgent::requestTimeout()
{
    setConnectionRequestType("Clear");
}

void UserAgent::sendConnectReply(const QString &replyMessage, int timeout)
{
    setConnectionRequestType(replyMessage);
    QTimer::singleShot(timeout * 1000, this,SLOT(requestTimeout()));
}

void UserAgent::updateMgrAvailability(bool available)
{
    if (available) {
        m_manager->registerAgent(QString(AGENT_PATH));
    }
}

void UserAgent::setConnectionRequestType(const QString &type)
{
    if (type == "Suppress") {
        requestType = TYPE_SUPPRESS;
    } else if (type == "Clear") {
        requestType = TYPE_CLEAR;
    } else {
        requestType = TYPE_DEFAULT;
    }
}

QString UserAgent::connectionRequestType() const
{
    switch (requestType) {
    case TYPE_SUPPRESS:
        return "Suppress";
        break;
    case TYPE_CLEAR:
        return "Clear";
        break;
    default:
        break;
    }
    return QString();
}

void UserAgent::requestConnect(const QDBusMessage &msg)
{
    QList<QVariant> arguments;
    arguments << QVariant(connectionRequestType());
    QDBusMessage error = msg.createReply(arguments);

    if (!QDBusConnection::systemBus().send(error)) {
        qDebug() << "Could not queue message";
    }
    if (connectionRequestType() == "Suppress") {
        return;
    }

    setConnectionRequestType("Suppress");
    Q_EMIT userConnectRequested(msg);
    Q_EMIT connectionRequest();
}

////////////////////
AgentAdaptor::AgentAdaptor(UserAgent* parent)
  : QDBusAbstractAdaptor(parent),
    m_userAgent(parent)
{
}

AgentAdaptor::~AgentAdaptor()
{
}

void AgentAdaptor::Release()
{
}

void AgentAdaptor::ReportError(const QDBusObjectPath &service_path, const QString &error)
{
    Q_UNUSED(service_path)
    m_userAgent->reportError(error);
}

void AgentAdaptor::RequestBrowser(const QDBusObjectPath &service_path, const QString &url)
{
    pr_dbg() << "Service " << service_path.path() << " wants browser to open hotspot's url " << url;
}

void AgentAdaptor::RequestInput(const QDBusObjectPath &service_path,
                                       const QVariantMap &fields,
                                       const QDBusMessage &message)
{
    QVariantMap json;
    foreach (const QString &key, fields.keys()){
        QVariantMap payload = qdbus_cast<QVariantMap>(fields[key]);
        json.insert(key, payload);
    }

    message.setDelayedReply(true);

    ServiceRequestData *reqdata = new ServiceRequestData;
    reqdata->objectPath = service_path.path();
    reqdata->fields = json;
    reqdata->reply = message.createReply();
    reqdata->msg = message;

    m_userAgent->requestUserInput(reqdata);
}

void AgentAdaptor::Cancel()
{
    m_userAgent->cancelUserInput();
}

void AgentAdaptor::RequestConnect(const QDBusMessage &message)
{
    m_userAgent->requestConnect(message);
}
