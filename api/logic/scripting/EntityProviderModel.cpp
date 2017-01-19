#include "EntityProviderModel.h"

#include "EntityProviderManager.h"
#include "BaseVersion.h"
#include "BaseVersionList.h"

struct BaseNode
{
	virtual ~BaseNode()
	{
		qDeleteAll(children);
	}

	BaseNode *parent = nullptr;
	QVector<BaseNode *> children;

	void addChild(BaseNode *n)
	{
		n->parent = this;
		children.append(n);
	}

	bool isRootNode() const { return parent == nullptr; }
	bool isProviderNode() const;
	bool isEntityNode() const;
	bool isVersionNode() const;
};
struct RootNode : public BaseNode
{
};
struct ProviderNode : public BaseNode
{
	EntityProvider *provider;
};
struct EntityNode : public BaseNode
{
	EntityProvider::Entity entity;
	std::shared_ptr<BaseVersionList> versionList;
};
struct VersionNode : public BaseNode
{
	BaseVersionPtr version;
};

bool BaseNode::isProviderNode() const { return !!dynamic_cast<const ProviderNode *>(this); }
bool BaseNode::isEntityNode() const { return !!dynamic_cast<const EntityNode *>(this); }
bool BaseNode::isVersionNode() const { return !!dynamic_cast<const VersionNode *>(this); }


EntityProviderModel::EntityProviderModel(EntityProviderManager *manager)
	: QAbstractItemModel(manager), m_manager(manager), m_base(new RootNode) {}

EntityProviderModel::~EntityProviderModel()
{
	delete m_base;
}

QModelIndex EntityProviderModel::index(int row, int column, const QModelIndex &parent) const
{
	return createIndex(row, column, node(parent)->children.at(row));
}
QModelIndex EntityProviderModel::parent(const QModelIndex &child) const
{
	return indexFor(node(child)->parent);
}

int EntityProviderModel::rowCount(const QModelIndex &parent) const
{
	return node(parent)->children.size();
}
int EntityProviderModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QVariant EntityProviderModel::data(const QModelIndex &index, int role) const
{
	BaseNode *n = node(index);
	if (n->isProviderNode())
	{
		if (role == Qt::DisplayRole && index.column() == 0)
		{
			return static_cast<ProviderNode *>(n)->provider->id();
		}
	}
	else if (n->isEntityNode())
	{
		if (role == Qt::DisplayRole && index.column() == 0)
		{
			return static_cast<EntityNode *>(n)->entity.name;
		}
		else if (role == Qt::ToolTipRole)
		{
			return static_cast<EntityNode *>(n)->entity.internalId;
		}
	}
	else if (n->isVersionNode())
	{
		if (role == Qt::DisplayRole && index.column() == 0)
		{
			return static_cast<VersionNode *>(n)->version->name();
		}
	}
	return QVariant();
}
QVariant EntityProviderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return QVariant();
}

void EntityProviderModel::notifyAfterProviderAdd(EntityProvider *provider)
{
	auto setupVersionList = [this](EntityNode *node)
	{
		node->versionList = node->entity.provider->versionList(node->entity);

		auto addAllVersions = [this, node]()
		{
			for (int i = 0; i < node->versionList->count(); ++i)
			{
				VersionNode *vNode = new VersionNode;
				vNode->version = node->versionList->at(i);
				node->addChild(vNode);
			}
		};
		addAllVersions();

		auto versionListReset = [this, node, addAllVersions]()
		{
			beginInsertRows(indexFor(node), 0, node->versionList->count());
			addAllVersions();
			endInsertRows();
		};

		connect(node->versionList.get(), &BaseVersionList::rowsInserted, this, [this, node](QModelIndex, int from, int to)
		{
			beginInsertRows(indexFor(node), node->children.size(), node->children.size() + (to - from));
			for (int i = from; i < to; ++i)
			{
				VersionNode *vNode = new VersionNode;
				vNode->version = node->versionList->at(i);
				node->addChild(vNode);
			}
			endInsertRows();
		});
		connect(node->versionList.get(), &BaseVersionList::rowsAboutToBeRemoved, this, [this, node](QModelIndex, int from, int to)
		{
			for (int i = from; i < to; ++i)
			{
				BaseVersionPtr version = node->versionList->at(i);
				for (BaseNode *vNode : node->children)
				{
					if (static_cast<VersionNode *>(vNode)->version == version)
					{
						const int index = node->children.indexOf(vNode);
						beginRemoveRows(indexFor(node), index, index);
						delete node->children.takeAt(index);
						endRemoveRows();
						break;
					}
				}
			}
		});
		connect(node->versionList.get(), &BaseVersionList::modelAboutToBeReset, this, [this, node]()
		{
			beginRemoveRows(indexFor(node), 0, node->children.size());
			qDeleteAll(node->children);
			node->children.clear();
			endRemoveRows();
		});
		connect(node->versionList.get(), &BaseVersionList::modelReset, this, versionListReset);
	};

	ProviderNode *node = new ProviderNode;
	node->provider = provider;

	beginInsertRows(indexFor(m_base), rowCount(), rowCount());
	{
		m_base->addChild(node);

		for (const EntityProvider::Entity &entity : provider->entities())
		{
			EntityNode *n = new EntityNode;
			n->entity = entity;
			node->addChild(n);
			setupVersionList(n);
		}
	}
	endInsertRows();

	connect(provider, &EntityProvider::beforeEntitiesUpdate, this, [this, node, provider]()
	{
		beginRemoveRows(indexFor(node), 0, node->children.size()-1);
		qDeleteAll(node->children);
		node->children.clear();
		endRemoveRows();
	});
	connect(provider, &EntityProvider::afterEntitiesUpdate, this, [this, node, provider, setupVersionList]()
	{
		beginInsertRows(indexFor(node), 0, provider->entities().size());
		for (const EntityProvider::Entity &entity : provider->entities())
		{
			EntityNode *n = new EntityNode;
			n->entity = entity;
			node->addChild(n);
			setupVersionList(n);
		}
		endInsertRows();
	});
}
void EntityProviderModel::notifyBeforeProviderRemove(EntityProvider *provider)
{
	disconnect(provider, 0, this, 0);

	int index;
	for (int i = 0; i < m_base->children.size(); ++i)
	{
		if (static_cast<ProviderNode *>(m_base->children.at(i))->provider == provider)
		{
			index = i;
			break;
		}
	}
	beginRemoveRows(indexFor(m_base), index, index);
	m_base->children.removeAt(index);
	endRemoveRows();
}

EntityProvider *EntityProviderModel::providerForIndex(const QModelIndex &index)
{
	BaseNode *n = node(index);
	if (n->isProviderNode())
	{
		return static_cast<ProviderNode *>(n)->provider;
	}
	else
	{
		return nullptr;
	}
}
EntityProvider::Entity EntityProviderModel::entityForIndex(const QModelIndex &index)
{
	BaseNode *n = node(index);
	if (n->isEntityNode())
	{
		return static_cast<EntityNode *>(n)->entity;
	}
	else
	{
		return {};
	}
}

BaseNode *EntityProviderModel::node(const QModelIndex &index) const
{
	return index.isValid() ? static_cast<BaseNode *>(index.internalPointer()) : m_base;
}
QModelIndex EntityProviderModel::indexFor(BaseNode *node, const int column) const
{
	if (node == m_base || node == nullptr)
	{
		return QModelIndex();
	}
	return createIndex(node->parent->children.indexOf(node), column, node);
}
