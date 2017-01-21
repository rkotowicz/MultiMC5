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
	const QVariant source = QIdentityProxyModel::data(proxyIndex, role);
	if (role == Qt::DecorationRole && (source.type() == QVariant::String || source.type() == QVariant::Url))
	{
		const QUrl url = source.toUrl();
		if (!url.isValid() || url.isEmpty())
		{
			return QVariant();
		}

		auto read = [](const QString &filename) -> QVariant
		{
			if (filename.endsWith(".ico"))
			{
				return QIcon(filename);
			}
			else
			{
				return QPixmap(filename).scaled(32, 32, Qt::KeepAspectRatio);
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
				m_cache.insert(cacheKey, QVariant()); // prevent multiple downloads

				QPersistentModelIndex persistentIndex(proxyIndex);

				NetJob *job = new NetJob(tr("Icon download"));
				auto dl = Net::Download::makeCached(url, entry);
				connect(dl.get(), &Net::Download::succeeded, this, [this, cacheKey, entry, persistentIndex, read]()
				{
					m_cache.insert(cacheKey, read(entry->getFullPath()));
					if (persistentIndex.isValid())
					{
						emit const_cast<IconUrlProxyModel *>(this)->dataChanged(persistentIndex, persistentIndex, QVector<int>() << Qt::DecorationRole);
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
		return m_cache.value(cacheKey);
	}
	return source;
}
