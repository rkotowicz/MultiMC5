#pragma once

#include <QIdentityProxyModel>
#include <QHash>

#include "multimc_logic_export.h"

class NetJob;

class MULTIMC_LOGIC_EXPORT IconUrlProxyModel : public QIdentityProxyModel
{
public:
	explicit IconUrlProxyModel(QObject *parent = nullptr);

	QVariant data(const QModelIndex &proxyIndex, int role) const override;

private:
	mutable QHash<QString, QVariant> m_cache;
};
