#include "Mirror.h"
#include "Json.h"

Mirror::Mirror(const QByteArray& json)
{
	using namespace Json;
	try
	{
		const auto doc = requireDocument(json, "Mirror Json");
		const auto mainObj = requireObject(doc, "Main Object");
		const auto version = requireInteger(mainObj, "version");
		if(version != 1)
		{
			throw 1;
		}
		m_name = requireString(mainObj, "name");
		const auto array = requireArray(mainObj, "mirrors");
		for(const auto & arrayItem : array)
		{
			const auto arrayObject = requireObject(arrayItem);
			QString name = requireString(arrayObject, "name");
			QString from = requireString(arrayObject, "officialPrefix");
			QString to = requireString(arrayObject, "mirrorPrefix");
			add(name, from, to);
		}
	}
	catch(...)
	{
		broken = true;
	}
}


Mirror::Mirror()
{
	broken = false;
}

void Mirror::add(const QString& name, const QString& sourcePrefix, const QString& targetPrefix)
{
	entries.append({name, sourcePrefix, targetPrefix});
}

void Mirror::clear()
{
	entries.clear();
	broken = false;
}

QStringList Mirror::lookup(QString url)
{
	if(broken)
	{
		return {url};
	}
	QStringList result;
	for (auto & entry: entries)
	{
		if(url.startsWith(entry.sourcePrefix))
		{
			auto newUrl = url;
			result.append(newUrl.replace(0, entry.sourcePrefix.length(), entry.targetPrefix));
		}
	}
	// append the original URL anyway
	result.append(url);
	return result;
}
