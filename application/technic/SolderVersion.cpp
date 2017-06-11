#include "SolderVersion.h"
#include "minecraft/VersionFilterData.h"
#include <QObject>
#include "Version.h"

QString SolderVersion::name()
{
	return id;
}

QString SolderVersion::descriptor()
{
	return id;
}

QString SolderVersion::typeString() const
{
	if (is_latest)
		return QObject::tr("Latest");

	if (is_recommended)
		return QObject::tr("Recommended");

	return QString();
}

bool SolderVersion::operator<(BaseVersion &a)
{
	SolderVersion *pa = dynamic_cast<SolderVersion *>(&a);
	if (!pa)
		return true;

	return version < pa->version;
}

bool SolderVersion::operator>(BaseVersion &a)
{
	SolderVersion *pa = dynamic_cast<SolderVersion *>(&a);
	if (!pa)
		return false;

	return version > pa->version;
}

QString SolderVersion::filename()
{
	return QString();
}

QString SolderVersion::url()
{
	return base_url + pack_name + "/" + id;
}

SolderVersion::SolderVersion(QString versionId)
{
	id = versionId;
	auto copyId = versionId;
	if(copyId.startsWith('v'))
	{
		copyId.remove(0,1);
	}
	version = Version(copyId);
}

SolderVersion::SolderVersion(QJsonObject version)
{
	SolderVersion(version.value("version").toString());

	base_url = version.value("base_url").toString();
	pack_name = version.value("pack_name").toString();
}

QJsonObject SolderVersion::toJson()
{
	QJsonObject obj;
	obj.insert("base_url", base_url);
	obj.insert("pack_name", pack_name);
	obj.insert("version", id);
	return obj;
}
