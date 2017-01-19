#include "LuaUtil.h"

#include "Json.h"

QVariantHash LuaUtil::toVariantHash(const sol::table &table)
{
	QVariantHash out;
	for (auto it = table.begin(); it != table.end(); ++it)
	{
		const auto pair = *it;
		if (pair.first.get_type() == sol::type::number)
		{
			out.insert(QString::number(pair.first.as<int>()), toVariant(pair.second));
		}
		else
		{
			out.insert(LuaUtil::required<QString>(pair.first),
					   toVariant(pair.second));
		}
	}
	return out;
}
QVariantList LuaUtil::toVariantList(const sol::table &table)
{
	QVariantList out;
	for (auto it = table.begin(); it != table.end(); ++it)
	{
		out.append(toVariant((*it).second));
	}
	return out;
}
QVariant LuaUtil::toVariant(const sol::object &object)
{
	const sol::type type = object.get_type();
	switch (type)
	{
	case sol::type::none:
	case sol::type::nil:
		return QVariant();
	case sol::type::string: return QString::fromStdString(object.as<std::string>());
	case sol::type::number: return object.as<double>();
	case sol::type::boolean: return object.as<bool>();
	case sol::type::table:
	{
		const sol::table table = object.as<sol::table>();
		if (isMap(table))
		{
			return toVariantHash(table);
		}
		else
		{
			return toVariantList(table);
		}
	}
	default: return QVariant(QVariant::Invalid);
	}
}

sol::table LuaUtil::fromJson(sol::state &state, const QJsonObject &obj)
{
	sol::table table = state.create_table(0, obj.size());
	for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
	{
		const std::string key = it.key().toStdString();
		const QJsonValue value = it.value();
		switch (value.type())
		{
		case QJsonValue::Null: table[key] = sol::nil; break;
		case QJsonValue::Bool: table[key] = value.toBool(); break;
		case QJsonValue::Double: table[key] = value.toDouble(); break;
		case QJsonValue::String: table[key] = value.toString().toStdString(); break;
		case QJsonValue::Array: table[key] = fromJson(state, value.toArray()); break;
		case QJsonValue::Object: table[key] = fromJson(state, value.toObject()); break;
		case QJsonValue::Undefined:
			break;
		}
	}
	return table;
}
sol::table LuaUtil::fromJson(sol::state &state, const QJsonArray &arr)
{
	sol::table table = state.create_table(arr.size(), 0);
	for (int i = 0; i < arr.size(); ++i)
	{
		const QJsonValue value = arr.at(i);
		switch (value.type())
		{
		case QJsonValue::Null: table[i+1] = sol::nil; break;
		case QJsonValue::Bool: table[i+1] = value.toBool(); break;
		case QJsonValue::Double: table[i+1] = value.toDouble(); break;
		case QJsonValue::String: table[i+1] = value.toString().toStdString(); break;
		case QJsonValue::Array: table[i+1] = fromJson(state, value.toArray()); break;
		case QJsonValue::Object: table[i+1] = fromJson(state, value.toObject()); break;
		case QJsonValue::Undefined:
			break;
		}
	}
	return table;
}
sol::table LuaUtil::fromJson(sol::state &state, const std::string &json)
{
	const QJsonDocument doc = Json::requireDocument(QByteArray::fromStdString(json));

	if (doc.isArray())
	{
		return fromJson(state, doc.array());
	}
	else
	{
		return fromJson(state, doc.object());
	}
}
std::string LuaUtil::toJson(const sol::table &table)
{
	return Json::toText(toJsonObject(table)).toStdString();
}

QJsonObject LuaUtil::toJsonObject(const sol::table &table)
{
	// lazy...
	return QJsonObject::fromVariantHash(toVariantHash(table));
}

bool LuaUtil::isMap(const sol::table &table)
{
	static bool nodebug = false;

	if (table.cbegin() == table.cend())
	{
		return false; // ouch, no way to tell, assume it isn't
	}
	auto it = table.cbegin();
	return (*it).first.get_type() != sol::type::number;
}
