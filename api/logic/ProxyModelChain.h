#pragma once

#include <QObject>
#include <QVector>

#include "multimc_logic_export.h"

class QAbstractProxyModel;
class QAbstractItemModel;
class QModelIndex;

class MULTIMC_LOGIC_EXPORT ProxyModelChain : public QObject
{
public:
	explicit ProxyModelChain(QObject *parent = nullptr);

	void append(QAbstractProxyModel *proxy);
	void prepend(QAbstractProxyModel *proxy);

	QModelIndex mapToSource(const QModelIndex &index) const;
	QModelIndex mapFromSource(const QModelIndex &index) const;

	void setSourceModel(QAbstractItemModel *source);
	QAbstractProxyModel *get() const;

private:
	QAbstractItemModel *m_source = nullptr;
	QVector<QAbstractProxyModel *> m_chain;
};
