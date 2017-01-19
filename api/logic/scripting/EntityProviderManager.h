#pragma once

#include <QObject>
#include <QVector>
#include <memory>

#include "multimc_logic_export.h"
#include "EntityProvider.h"

class Task;
class EntityProviderModel;
class EntityModel;
class QAbstractItemModel;
class BaseVersionList;
class BaseVersion;

class MULTIMC_LOGIC_EXPORT EntityProviderManager : public QObject
{
	Q_OBJECT
public:
	explicit EntityProviderManager(QObject *parent = nullptr);

	void registerProvider(EntityProvider *provider);

	QVector<EntityProvider *> providers() const { return m_providers; }
	EntityProvider *provider(const QString &id) const;

	std::shared_ptr<BaseVersionList> versionList(const QString &id) const;
	std::unique_ptr<Task> createInstallTask(BaseInstance *instance, const std::shared_ptr<BaseVersion> &version);

	std::unique_ptr<Task> createUpdateAllTask();

	QAbstractItemModel *entitiesModel() const;

	QAbstractItemModel *infoModel() const;
	EntityProvider *providerForIndex(const QModelIndex &index) const;
	EntityProvider::Entity entityForIndex(const QModelIndex &index) const;

private:
	QVector<EntityProvider *> m_providers;
	EntityProviderModel *m_model;
	EntityModel *m_entityModel;

	std::pair<EntityProvider *, EntityProvider::Entity> findProviderOf(const QString &id) const;
};
