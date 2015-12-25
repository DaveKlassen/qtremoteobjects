/****************************************************************************
**
** Copyright (C) 2014 Ford Motor Company
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QREMOTEOBJECTNODE_H
#define QREMOTEOBJECTNODE_H

#include <QtCore/QSharedPointer>
#include <QtRemoteObjects/qtremoteobjectglobal.h>
#include <QtRemoteObjects/qremoteobjectregistry.h>
#include <QtRemoteObjects/qremoteobjectdynamicreplica.h>

QT_BEGIN_NAMESPACE

class QRemoteObjectReplica;
class SourceApiMap;
class QAbstractItemModel;
class QAbstractItemReplica;
class QItemSelectionModel;
class QRemoteObjectNodePrivate;
class QRemoteObjectHostBasePrivate;
class QRemoteObjectHostPrivate;
class QRemoteObjectRegistryHostPrivate;
class ClientIoDevice;

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectNode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl registryUrl READ registryUrl WRITE setRegistryUrl)
public:

    enum ErrorCode{
        NoError,
        RegistryNotAcquired,
        RegistryAlreadyHosted,
        NodeIsNoServer,
        ServerAlreadyCreated,
        UnintendedRegistryHosting,
        OperationNotValidOnClientNode,
        SourceNotRegistered,
        MissingObjectName,
        HostUrlInvalid
    };

    QRemoteObjectNode(QObject *parent = 0);
    QRemoteObjectNode(const QUrl &registryAddress, QObject *parent = 0);
    virtual ~QRemoteObjectNode();

    Q_INVOKABLE bool connectToNode(const QUrl &address);
    virtual void setName(const QString &name);
    template < class ObjectType >
    ObjectType *acquire(const QString &name = QString())
    {
        ObjectType* replica = new ObjectType;
        return qobject_cast< ObjectType* >(acquire(&ObjectType::staticMetaObject, replica, name));
    }
    QRemoteObjectDynamicReplica *acquire(const QString &name);
    QAbstractItemReplica *acquireModel(const QString &name);
    QUrl registryUrl() const;
    virtual bool setRegistryUrl(const QUrl &registryAddress);
    bool waitForRegistry(int timeout = 30000);
    const QRemoteObjectRegistry *registry() const;

    ErrorCode lastError() const;

    void timerEvent(QTimerEvent*);

Q_SIGNALS:
    void remoteObjectAdded(const QRemoteObjectSourceLocation &);
    void remoteObjectRemoved(const QRemoteObjectSourceLocation &);

protected:
    QRemoteObjectNode(QRemoteObjectNodePrivate &, QObject *parent);

private:
    QRemoteObjectReplica *acquire(const QMetaObject *, QRemoteObjectReplica *, const QString &name = QString());
    Q_DECLARE_PRIVATE(QRemoteObjectNode)
    Q_PRIVATE_SLOT(d_func(), void onClientRead(QObject *obj))
    Q_PRIVATE_SLOT(d_func(), void onRemoteObjectSourceAdded(const QRemoteObjectSourceLocation &entry))
    Q_PRIVATE_SLOT(d_func(), void onRemoteObjectSourceRemoved(const QRemoteObjectSourceLocation &entry))
    Q_PRIVATE_SLOT(d_func(), void onRegistryInitialized())
    Q_PRIVATE_SLOT(d_func(), void onShouldReconnect(ClientIoDevice *ioDevice))
};

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectHostBase : public QRemoteObjectNode
{
    Q_OBJECT
public:
    void setName(const QString &name) Q_DECL_OVERRIDE;

    template <template <typename> class ApiDefinition, typename ObjectType>
    bool enableRemoting(ObjectType *object)
    {
        ApiDefinition<ObjectType> *api = new ApiDefinition<ObjectType>;
        return enableRemoting(object, api);
    }
    bool enableRemoting(QObject *object, const QString &name = QString());
    bool enableRemoting(QAbstractItemModel *model, const QString &name, const QVector<int> roles, QItemSelectionModel *selectionModel = 0);
    bool disableRemoting(QObject *remoteObject);

protected:
    virtual QUrl hostUrl() const;
    virtual bool setHostUrl(const QUrl &hostAddress);
    QRemoteObjectHostBase(QRemoteObjectHostBasePrivate &, QObject *);

private:
    bool enableRemoting(QObject *object, const SourceApiMap *, QObject *adapter=0);
    Q_DECLARE_PRIVATE(QRemoteObjectHostBase)
};

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectHost : public QRemoteObjectHostBase
{
    Q_OBJECT
public:
    QRemoteObjectHost(QObject *parent = Q_NULLPTR);
    QRemoteObjectHost(const QUrl &address, const QUrl &registryAddress = QUrl(), QObject *parent = Q_NULLPTR);
    QRemoteObjectHost(const QUrl &address, QObject *parent);
    virtual ~QRemoteObjectHost();
    QUrl hostUrl() const Q_DECL_OVERRIDE;
    bool setHostUrl(const QUrl &hostAddress) Q_DECL_OVERRIDE;

protected:
    QRemoteObjectHost(QRemoteObjectHostPrivate &, QObject *);

private:
    Q_DECLARE_PRIVATE(QRemoteObjectHost)
};

class Q_REMOTEOBJECTS_EXPORT QRemoteObjectRegistryHost : public QRemoteObjectHostBase
{
    Q_OBJECT
public:
    QRemoteObjectRegistryHost(const QUrl &registryAddress = QUrl(), QObject *parent = Q_NULLPTR);
    virtual ~QRemoteObjectRegistryHost();
    bool setRegistryUrl(const QUrl &registryUrl) Q_DECL_OVERRIDE;

protected:
    QRemoteObjectRegistryHost(QRemoteObjectRegistryHostPrivate &, QObject *);

private:
    Q_DECLARE_PRIVATE(QRemoteObjectRegistryHost)
};

QT_END_NAMESPACE

#endif
