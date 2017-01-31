#pragma once

#include <QIdentityProxyModel>
#include <QHash>
#include <QPixmapCache>

#include "multimc_logic_export.h"

class NetJob;

class MULTIMC_LOGIC_EXPORT IconUrlProxyModel : public QIdentityProxyModel
{
public:
	explicit IconUrlProxyModel(QObject *parent = nullptr);

	enum
	{
		SourceSizeDecorationRole = Qt::UserRole + 1010
	};

	QVariant data(const QModelIndex &proxyIndex, int role) const override;

private:
	mutable QHash<QString, QPair<QVariant, QVariant>> m_cache;
};
