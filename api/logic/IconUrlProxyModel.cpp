#include "IconUrlProxyModel.h"

#include <QPixmap>
#include <QIcon>

#include "net/Download.h"
#include "net/NetJob.h"
#include "net/HttpMetaCache.h"
#include "Env.h"

IconUrlProxyModel::IconUrlProxyModel(QObject *parent)
	: QIdentityProxyModel(parent) {}

QVariant IconUrlProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
	const QVariant source = QIdentityProxyModel::data(proxyIndex,
													  role == SourceSizeDecorationRole ? Qt::DecorationRole : role);
	if ((role == Qt::DecorationRole || role == SourceSizeDecorationRole) &&
			(source.type() == QVariant::String || source.type() == QVariant::Url))
	{
		const QUrl url = source.toUrl();
		if (!url.isValid() || url.isEmpty())
		{
			return QVariant();
		}

		auto read = [](const QString &filename) -> QPair<QVariant, QVariant>
		{
			if (filename.endsWith(".ico"))
			{
				const QIcon icon = QIcon(filename);
				return QPair<QVariant, QVariant>(icon, icon);
			}
			else
			{
				const QPixmap orig = QPixmap(filename);
				return QPair<QVariant, QVariant>(orig, orig.scaled(32, 32, Qt::KeepAspectRatio));
			}
		};

		const QString cacheKey = url.toString(QUrl::FullyEncoded);
		if (!m_cache.contains(cacheKey) && url.isLocalFile())
		{
			m_cache.insert(cacheKey, read(url.toLocalFile()));
		}
		else if (!m_cache.contains(cacheKey))
		{
			auto entry = ENV.metacache()->resolveEntry("icons", cacheKey);
			if (entry->isStale())
			{
				m_cache.insert(cacheKey, qMakePair(QVariant(), QVariant())); // prevent multiple downloads

				QPersistentModelIndex persistentIndex(proxyIndex);

				NetJob *job = new NetJob(tr("Icon download"));
				auto dl = Net::Download::makeCached(url, entry);
				connect(dl.get(), &Net::Download::succeeded, this, [this, cacheKey, entry, persistentIndex, read]()
				{
					m_cache.insert(cacheKey, read(entry->getFullPath()));
					if (persistentIndex.isValid())
					{
						emit const_cast<IconUrlProxyModel *>(this)->dataChanged(persistentIndex, persistentIndex,
																				QVector<int>() << Qt::DecorationRole << SourceSizeDecorationRole);
					}
				});
				job->addNetAction(dl);
				connect(job, &NetJob::finished, job, &NetJob::deleteLater);
				job->start();
			}
			else
			{
				m_cache.insert(cacheKey, read(entry->getFullPath()));
			}
		}

		if (role == Qt::DecorationRole)
		{
			return m_cache.value(cacheKey).second;
		}
		else
		{
			return m_cache.value(cacheKey).first;
		}
	}
	return source;
}
