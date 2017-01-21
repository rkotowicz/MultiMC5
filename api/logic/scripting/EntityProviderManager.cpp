#include "EntityProviderManager.h"

#include <QAbstractItemModel>

#include "tasks/SequentialTask.h"
#include "ScriptEntityVersion.h"
#include "ScriptEntityVersionList.h"
#include "BaseVersionList.h"
#include "EntityProvider.h"
#include "EntityProviderModel.h"

class EntityModel : public QAbstractListModel
{
public:
	explicit EntityModel(EntityProviderManager *manager)
		: QAbstractListModel(manager), m_manager(manager) {}

	int rowCount(const QModelIndex & = QModelIndex()) const override
	{
		return m_entities.size();
	}
	int columnCount(const QModelIndex &) const override
	{
		return 2;
	}
	QVariant data(const QModelIndex &index, int role) const override
	{
		const EntityProvider::Entity entity = m_entities.at(index.row());
		if (role == Qt::DisplayRole)
		{
			switch (index.column())
			{
			case 0: return entity.name;
			case 1: return entity.provider->id();
			}
		}
		else if (role == Qt::ToolTipRole)
		{
			return entity.internalId;
		}
		else if (role == Qt::DecorationRole && index.column() == 0)
		{
			return entity.iconUrl;
		}
		return QVariant();
	}
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override
	{
		if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		{
			switch (section)
			{
			case 0: return tr("Name");
			case 1: return tr("Provider");
			}
		}
		return QVariant();
	}

	void notifyAfterProviderAdded(EntityProvider *provider)
	{
		addEntitiesFor(provider);
		connect(provider, &EntityProvider::beforeEntitiesUpdate, this, [this, provider]() { removeEntitiesFor(provider); });
		connect(provider, &EntityProvider::afterEntitiesUpdate, this, [this, provider]() { addEntitiesFor(provider); });
	}
	void notifyBeforeProviderRemoved(EntityProvider *provider)
	{
		disconnect(provider, 0, this, 0);
		removeEntitiesFor(provider);
	}

	EntityProvider::Entity entityForIndex(const QModelIndex &index)
	{
		return m_entities.at(index.row());
	}

private:
	EntityProviderManager *m_manager;
	QVector<EntityProvider::Entity> m_entities;

	void removeEntitiesFor(EntityProvider *provider)
	{
		for (int i = 0; i < m_entities.size(); ++i)
		{
			if (m_entities.at(i).provider == provider)
			{
				beginRemoveRows(QModelIndex(), i, i);
				m_entities.removeAt(i);
				endRemoveRows();
			}
		}
	}
	void addEntitiesFor(EntityProvider *provider)
	{
		if (provider->entities().isEmpty())
		{
			return;
		}
		beginInsertRows(QModelIndex(), rowCount(), rowCount() + provider->entities().size() - 1);
		m_entities.append(provider->entities());
		endInsertRows();
	}
};

EntityProviderManager::EntityProviderManager(QObject *parent)
	: QObject(parent), m_model(new EntityProviderModel(this)), m_entityModel(new EntityModel(this)) {}

void EntityProviderManager::registerProvider(EntityProvider *provider)
{
	connect(provider, &EntityProvider::destroyed, this, [this, provider]()
	{
		m_model->notifyBeforeProviderRemove(provider);
		m_entityModel->notifyBeforeProviderRemoved(provider);
		m_providers.removeAll(provider);
	});
	m_providers.append(provider);
	m_model->notifyAfterProviderAdd(provider);
	m_entityModel->notifyAfterProviderAdded(provider);
}

EntityProvider *EntityProviderManager::provider(const QString &id) const
{
	for (EntityProvider *ep : m_providers)
	{
		if (ep->id() == id)
		{
			return ep;
		}
	}
	return nullptr;
}

std::shared_ptr<BaseVersionList> EntityProviderManager::versionList(const QString &id) const
{
	const auto pair = findProviderOf(id);
	if (!pair.first)
	{
		return nullptr;
	}
	return pair.first->versionList(pair.second);
}
std::unique_ptr<Task> EntityProviderManager::createInstallTask(BaseInstance *instance, const std::shared_ptr<BaseVersion> &version)
{
	ScriptVersionPtr ver = std::dynamic_pointer_cast<ScriptEntityVersion>(version);
	if (!ver)
	{
		return nullptr;
	}

	// 42 levels of indirection later...
	return ver->versionList()->entity().provider->createInstallTask(instance, version);
}

std::unique_ptr<Task> EntityProviderManager::createUpdateAllTask()
{
	auto task = std::make_unique<SequentialTask>();
	for (EntityProvider *ep : m_providers)
	{
		task->addTask(ep->createUpdateEntitiesTask());
	}
	return task;
}

QAbstractItemModel *EntityProviderManager::entitiesModel() const
{
	return m_entityModel;
}

QAbstractItemModel *EntityProviderManager::infoModel() const
{
	return m_model;
}
EntityProvider *EntityProviderManager::providerForIndex(const QModelIndex &index) const
{
	Q_ASSERT(index.isValid() && index.model() == m_model);
	return m_model->providerForIndex(index);
}

EntityProvider::Entity EntityProviderManager::entityForIndex(const QModelIndex &index) const
{
	Q_ASSERT(index.isValid());
	if (index.model() == m_model)
	{
		return m_model->entityForIndex(index);
	}
	else
	{
		return m_entityModel->entityForIndex(index);
	}
}

std::pair<EntityProvider *, EntityProvider::Entity> EntityProviderManager::findProviderOf(const QString &id) const
{
	for (EntityProvider *ep : m_providers)
	{
		for (const EntityProvider::Entity &ent : ep->entities())
		{
			if (ent.internalId == id)
			{
				return std::make_pair(ep, ent);
			}
		}
	}
	return std::make_pair<EntityProvider *, EntityProvider::Entity>(nullptr, EntityProvider::Entity{});
}
