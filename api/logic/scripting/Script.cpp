#include "Script.h"

#include <QEventLoop>

#include "FileSystem.h"
#include "LuaUtil.h"
#include "EntityProviderManager.h"
#include "EntityProvider.h"
#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"
#include "Json.h"
#include "Env.h"

Script::Script(const QString &filename, QObject *parent)
	: QObject(parent), m_lua(new sol::state), m_filename(filename)
{
	m_lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math);
	m_lua->set_function("register_entity_provider", [this](const sol::table &table)
	{
		EntityProvider *ep = new EntityProvider(table, this);
		m_epm->registerProvider(ep);
		m_scriptProvidedObjects.append(ep);
	});
}

void Script::reload()
{
	if (m_isLoaded)
	{
		unload();
	}
	load();
}

void Script::load()
{
	if (m_isLoaded) {
		return;
	}

	// TODO sandboxing: http://lua-users.org/wiki/SandBoxes
	// additionally only paths on the file system "that make sense" should be accessible to the script

	sol::load_result script = m_lua->load(FS::read(m_filename).toStdString());
	if (!script.valid()) {
		const sol::error err = script;
		throw ScriptLoadException(QString("Unable to load %1: %2").arg(m_filename, err.what()));
	}

	sol::protected_function func = script;

	auto res = func.call();
	if (!res.valid()) {
		const sol::error err = res;
		throw ScriptLoadException(QString("Unable to load %1: %2").arg(m_filename, err.what()));
	}

	try
	{
		m_updateUrl = LuaUtil::requiredString(*m_lua, "update_url");
		m_version = LuaUtil::requiredString(*m_lua, "version");
		m_name = LuaUtil::requiredString(*m_lua, "name");
		m_author = LuaUtil::requiredString(*m_lua, "author");
		m_id = LuaUtil::requiredString(*m_lua, "id");
	}
	catch (LuaException &e)
	{
		throw ScriptLoadException(QString("Unable to load %1: %2").arg(m_filename, e.cause()));
	}

	emit metadataUpdated();
	m_isLoaded = true;
}
void Script::unload()
{
	if (!m_isLoaded) {
		return;
	}

	qDeleteAll(m_scriptProvidedObjects);
	m_scriptProvidedObjects.clear();
	m_isLoaded = false;
}

void Script::setManagers(EntityProviderManager *epm)
{
	m_epm = epm;
}

ScriptTask::ScriptTask(const Function &f, Script *script, QObject *parent)
	: Task(parent), m_func(f), m_script(script) {}
void ScriptTask::executeTask()
{
	try
	{
		setStatus(tr("Running script..."));
		m_func(this);
		emitSucceeded();
	}
	catch (const Exception &e)
	{
		emitFailed(e.cause());
	}
	catch (const sol::error &e)
	{
		emitFailed(QString(e.what()));
	}
}

static void solDebug(const sol::object &obj)
{
	switch (obj.get_type())
	{
	case sol::type::string: qDebug() << QString::fromStdString(obj.as<std::string>()); break;
	case sol::type::boolean: qDebug() << obj.as<bool>(); break;
	case sol::type::number: qDebug() << obj.as<double>(); break;
	case sol::type::table: qDebug() << LuaUtil::toVariantHash(obj.as<sol::table>()); break;
	case sol::type::function: qDebug() << "sol::function()"; break;
	case sol::type::nil: qDebug() << "sol::nil()"; break;
	case sol::type::none: qDebug() << "sol::none()"; break;
	case sol::type::thread: qDebug() << "sol::thread()"; break;
	case sol::type::userdata: qDebug() << "sol::userdata()"; break;
	default: qDebug() << "sol::object()"; break;
	}
}

sol::table ScriptTask::taskContext()
{
	auto getCommon = [this](const Net::Download::Ptr &dl, const sol::table &opts)
	{
		auto addChecksumValidator = [dl, opts](const std::string &name, const QCryptographicHash::Algorithm algo)
		{
			const sol::optional<std::string> sum = opts[name];
			if (sum)
			{
				dl->addValidator(new Net::ChecksumValidator(algo, QByteArray::fromStdString(sum.value())));
			}
		};
		addChecksumValidator("md5", QCryptographicHash::Md5);
		addChecksumValidator("sha1", QCryptographicHash::Sha1);
		addChecksumValidator("sha224", QCryptographicHash::Sha224);
		addChecksumValidator("sha256", QCryptographicHash::Sha256);
		addChecksumValidator("sha384", QCryptographicHash::Sha384);
		addChecksumValidator("sha512", QCryptographicHash::Sha512);
		addChecksumValidator("sha3_224", QCryptographicHash::Sha3_224);
		addChecksumValidator("sha3_256", QCryptographicHash::Sha3_256);
		addChecksumValidator("sha3_384", QCryptographicHash::Sha3_384);
		addChecksumValidator("sha3_512", QCryptographicHash::Sha3_512);

		QEventLoop loop;

		NetJob job(QString::fromStdString(opts.get_or<std::string>("name", "Script download")));
		job.addNetAction(dl);
		connect(&job, &NetJob::progress, this, &ScriptTask::setProgress);
		connect(&job, &NetJob::status, this, &ScriptTask::setStatus);
		connect(&job, &NetJob::finished, &loop, &QEventLoop::quit);
		QMetaObject::invokeMethod(&job, "start", Qt::QueuedConnection);
		loop.exec();
		setProgress(0, 0);
		setStatus(tr("Running script..."));

		if (!job.successful())
		{
			throw Exception(job.failReason());
		}
	};
	auto getFile = [getCommon](const std::string &url, const std::string &destination, const sol::table &opts) -> void
	{
		const QString urlString = QString::fromStdString(url);

		Net::Download::Ptr dl;
		if (opts.get_or<bool, std::string, bool>("cached", true))
		{
			auto entry = ENV.metacache()->resolveEntry("general", urlString);
			entry->setStale(true);
			dl = Net::Download::makeCached(urlString, entry);

			getCommon(dl, opts);

			if (!QFile::copy(entry->getFullPath(), QString::fromStdString(destination)))
			{
				throw Exception(QString("Unable to copy %1 (%2) to %3").arg(entry->getFullPath(), urlString, QString::fromStdString(destination)));
			}
		}
		else
		{
			dl = Net::Download::makeFile(urlString, QString::fromStdString(destination));

			getCommon(dl, opts);
		}
	};
	auto get = [getCommon](const std::string &url, const sol::table &opts) -> std::string
	{
		const QString urlString = QString::fromStdString(url);

		Net::Download::Ptr dl;
		if (opts.get_or<bool, std::string, bool>("cached", true))
		{
			auto entry = ENV.metacache()->resolveEntry("general", urlString);
			entry->setStale(true);
			dl = Net::Download::makeCached(urlString, entry);
			getCommon(dl, opts);
			return FS::read(entry->getFullPath()).toStdString();
		}
		else
		{
			QByteArray out;
			dl = Net::Download::makeByteArray(urlString, &out);
			getCommon(dl, opts);
			return out.toStdString();
		}
	};

	sol::state *lua = m_script->m_lua;
	sol::table ctxt = m_script->staticContext();
	ctxt["status"] = lua->create_table_with(
				"message", sol::as_function([this](const std::string &msg) { setStatus(QString::fromStdString(msg)); }),
				"progress", sol::as_function([this](const uint64_t current, const uint64_t total) { setProgress(current, total); }),
				"progress_idle", sol::as_function([this]() { setProgress(0, 0); })
			);
	ctxt["http"] = lua->create_table_with(
				"get_file", sol::as_function(getFile),
				"get", sol::as_function(get)
			);
	ctxt["fs"] = lua->create_table_with();
	ctxt["from_json"] = sol::as_function([lua](const std::string &json) { return LuaUtil::fromJson(*lua, json); });
	ctxt["to_json"] = sol::as_function(&LuaUtil::toJson);

	return ctxt;
}

sol::table Script::staticContext()
{
	return m_lua->create_table_with(
				"debug", sol::as_function(&solDebug)
				);
}
