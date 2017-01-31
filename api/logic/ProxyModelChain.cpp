#include "ProxyModelChain.h"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>

ProxyModelChain::ProxyModelChain(QObject *parent)
	: QObject(parent) {}

void ProxyModelChain::append(QAbstractProxyModel *proxy)
{
	if (!m_chain.isEmpty())
	{
		proxy->setSourceModel(m_chain.last());
	}
	else if (m_source)
	{
		proxy->setSourceModel(m_source);
	}
	m_chain.append(proxy);
}
void ProxyModelChain::prepend(QAbstractProxyModel *proxy)
{
	if (!m_chain.isEmpty())
	{
		m_chain.first()->setSourceModel(proxy);
	}
	m_chain.prepend(proxy);

	proxy->setSourceModel(m_source);
}

QModelIndex ProxyModelChain::mapToSource(const QModelIndex &index) const
{
	if (m_chain.isEmpty())
	{
		return index;
	}

	Q_ASSERT(index.model() == m_chain.last());

	QModelIndex ind = index;
	for (auto it = m_chain.crbegin(); it != m_chain.crend(); ++it)
	{
		ind = (*it)->mapToSource(ind);
	}

	Q_ASSERT(ind.model() == m_source);
	return ind;
}
QModelIndex ProxyModelChain::mapFromSource(const QModelIndex &index) const
{
	if (m_chain.isEmpty())
	{
		return index;
	}

	Q_ASSERT(index.model() == m_source);

	QModelIndex ind = index;
	for (auto it = m_chain.cbegin(); it != m_chain.cend(); ++it)
	{
		ind = (*it)->mapFromSource(ind);
	}

	Q_ASSERT(ind.model() == m_chain.last());

	return ind;
}

void ProxyModelChain::setSourceModel(QAbstractItemModel *source)
{
	m_source = source;
	if (!m_chain.isEmpty())
	{
		m_chain.first()->setSourceModel(source);
	}
}

QAbstractProxyModel *ProxyModelChain::get() const
{
	return m_chain.last();
}
