#pragma once

#include <QAbstractItemModel>

#include "EntityProvider.h"

class EntityProviderManager;
struct RootNode;
struct BaseNode;

class EntityProviderModel : public QAbstractItemModel
{
public:
	explicit EntityProviderModel(EntityProviderManager *manager);
	~EntityProviderModel();

	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	void notifyAfterProviderAdd(EntityProvider *provider);
	void notifyBeforeProviderRemove(EntityProvider *provider);

	EntityProvider *providerForIndex(const QModelIndex &index);
	EntityProvider::Entity entityForIndex(const QModelIndex &index);

private:
	EntityProviderManager *m_manager;
	RootNode *m_base;

	BaseNode *node(const QModelIndex &index) const;
	QModelIndex indexFor(BaseNode *node, const int column = 0) const;
};
