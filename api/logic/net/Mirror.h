#pragma once

#include <QString>
#include <QVector>
#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT Mirror
{
public:
	Mirror(const QByteArray & json);
	Mirror();
	QStringList lookup (QString url);

private:
	void add(const QString &name, const QString &sourcePrefix, const QString &targetPrefix);
	void clear();

private:
	struct MirrorEntry
	{
		QString name;
		QString sourcePrefix;
		QString targetPrefix;
	};
	QVector<MirrorEntry> entries;
	QString m_name;
	bool broken = false;
};
