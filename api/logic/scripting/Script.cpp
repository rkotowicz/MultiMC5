#include "Script.h"

#include <QEventLoop>
#include <QRegularExpression>
#include <bzlib.h>
#include <stdio.h>

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
#include "minecraft/GradleSpecifier.h"
#include "minecraft/onesix/OneSixVersionFormat.h"
#include "quazip.h"
#include "quazipfile.h"

Script::Script(const QString &filename, QObject *parent)
	: QObject(parent), m_lua(new sol::state), m_filename(filename)
{
	m_lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math);
	m_lua->set_function("has_value", [](const sol::table &table, const sol::object &obj)
	{
		for (const auto &pair : table)
		{
			if (pair.second == obj)
			{
				return true;
			}
		}
		return false;
	});
	m_lua->set_function("register_entity_provider", [this](const sol::table &table)
	{
		EntityProvider *ep = new EntityProvider(table, this);
		m_epm->registerProvider(ep);
		m_scriptProvidedObjects.append(ep);
	});
	m_lua->new_enum("EntityType",
					"Mod", EntityProvider::Entity::Mod,
					"Modpack", EntityProvider::Entity::Modpack,
					"TexturePack", EntityProvider::Entity::TexturePack,
					"World", EntityProvider::Entity::World,
					"Patch", EntityProvider::Entity::Patch,
					"Other", EntityProvider::Entity::Other);
	m_lua->set_function("parse_iso8601", [](const std::string &date)
	{
		return QDateTime::fromString(QString::fromStdString(date), Qt::ISODate).toSecsSinceEpoch();
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

	sol::load_result script = m_lua->load_file(m_filename.toStdString());
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
	case sol::type::string: qDebug() << "lua:" << QString::fromStdString(obj.as<std::string>()); break;
	case sol::type::boolean: qDebug() << "lua:" << obj.as<bool>(); break;
	case sol::type::number: qDebug() << "lua:" << obj.as<double>(); break;
	case sol::type::table: qDebug() << "lua:" << LuaUtil::toVariantHash(obj.as<sol::table>()); break;
	case sol::type::function: qDebug() << "lua:" << "sol::function()"; break;
	case sol::type::nil: qDebug() << "lua:" << "sol::nil()"; break;
	case sol::type::none: qDebug() << "lua:" << "sol::none()"; break;
	case sol::type::thread: qDebug() << "lua:" << "sol::thread()"; break;
	case sol::type::userdata: qDebug() << "lua:" << "sol::userdata()"; break;
	default: qDebug() << "lua:" << "sol::object()"; break;
	}
}
static sol::object solRegexMatch(const QRegularExpressionMatch &match, sol::this_state state)
{
	if (match.hasMatch())
	{
		return sol::table::create_with(state.L,
									   "captured", sol::as_function(
										   [match](const std::string &name) { return match.captured(QString::fromStdString(name)).toStdString(); }),
									   "length", match.lastCapturedIndex()+1);
	}
	else
	{
		return sol::nil;
	}
}

static sol::object solRegexOne(const std::string &regex, const std::string &string, sol::this_state state)
{
	const QRegularExpression re(QString::fromStdString(regex));
	const QRegularExpressionMatch match = re.match(QString::fromStdString(string));
	return solRegexMatch(match, state);
}
static sol::object solRegexMany(const std::string &regex, const std::string &string, sol::this_state state)
{
	const QRegularExpression re(QString::fromStdString(regex), QRegularExpression::MultilineOption);
	auto it = std::make_shared<QRegularExpressionMatchIterator>(re.globalMatch(QString::fromStdString(string)));
	return sol::table::create_with(state.L,
								   "next", sol::as_function([it, state]() { return solRegexMatch(it->next(), state); }),
								   "has_next", sol::as_function([it]() { return it->hasNext(); }));
}

sol::table ScriptTask::taskContext(const QDir &contextDir)
{
	// TODO limit to where stuff can be copied/downloaded
	auto getCommon = [this](const Net::Download::Ptr &dl, const sol::table &opts)
	{
		auto addChecksumValidator = [dl, opts](const std::string &name, const QCryptographicHash::Algorithm algo)
		{
			const sol::optional<std::string> sum = opts[name];
			if (sum)
			{
				dl->addValidator(new Net::ChecksumValidator(algo, QByteArray::fromHex(QByteArray::fromStdString(sum.value()))));
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
	auto getFile = [getCommon](const std::string &url, const sol::table &opts) -> std::string
	{
		const QString urlString = QString::fromStdString(url);

		Net::Download::Ptr dl;
		auto entry = ENV.metacache()->resolveEntry("general", urlString);
		if (!opts.get_or<bool, std::string, bool>("no_refetch", true))
		{
			entry->setStale(true);
		}
		dl = Net::Download::makeCached(urlString, entry);

		getCommon(dl, opts);

		return entry->getFullPath().toStdString();
	};
	auto getFileAndCopy = [getCommon, getFile, contextDir](const std::string &url, const std::string &destination, const sol::table &opts) -> void
	{
		const QString dest = contextDir.absoluteFilePath(QString::fromStdString(destination));
		const QString urlString = QString::fromStdString(url);
		const QString filename = QString::fromStdString(getFile(url, opts));
		if (!QFile::copy(filename, dest))
		{
			throw Exception(QString("Unable to copy %1 (%2) to %3").arg(filename, urlString, QString::fromStdString(destination)));
		}
	};
	auto get = [getCommon](const std::string &url, const sol::table &opts) -> std::string
	{
		const QString urlString = QString::fromStdString(url);

		Net::Download::Ptr dl;
		if (opts.get_or<bool, std::string, bool>("cached", true))
		{
			auto entry = ENV.metacache()->resolveEntry("general", urlString);
			if (!opts.get_or<bool, std::string, bool>("no_refetch", true))
			{
				entry->setStale(true);
			}
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

	auto libraryFrom = [](const std::string &name, const QByteArray &data)
	{
		const GradleSpecifier spec(QString::fromStdString(name));
		auto cacheentry = ENV.metacache()->resolveEntry("libraries", spec.toPath());

		FS::write(cacheentry->getFullPath(), data);
		QCryptographicHash md5sum(QCryptographicHash::Md5);

		cacheentry->setStale(false);
		cacheentry->setMD5Sum(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex().constData());
		ENV.metacache()->updateEntry(cacheentry);
	};
	auto verifyOneSixLibraryFormat = [](const sol::table &json)
	{
		try
		{
			OneSixVersionFormat::libraryFromJson(QJsonObject::fromVariantHash(LuaUtil::toVariantHash(json)), "<from lua>");
			return true;
		}
		catch (...)
		{
			return false;
		}
	};
	auto verifyOneSixVersionFormat = [](const sol::table &json)
	{
		try
		{
			OneSixVersionFormat::versionFileFromJson(QJsonDocument(QJsonObject::fromVariantHash(LuaUtil::toVariantHash(json))),
													 "<from lua>", false);
			return true;
		}
		catch (...)
		{
			return false;
		}
	};
	auto openZip = [this](const std::string &filename, sol::this_state state)
	{
		std::shared_ptr<QuaZip> zip = std::make_shared<QuaZip>(QString::fromStdString(filename));
		if (!zip->open(QuaZip::mdUnzip))
		{
			throw Exception(QString("Unable to open Zip file %1").arg(zip->getZipName()));
		}

		auto readBinary = [zip](const std::string &filename)
		{
			if (!zip->setCurrentFile(QString::fromStdString(filename)))
			{
				throw Exception(QString("Zip error: %1 does not exist in %2").arg(QString::fromStdString(filename), zip->getZipName()));
			}
			QuaZipFile file(zip.get());
			if (!file.open(QuaZipFile::ReadOnly))
			{
				throw Exception(QString("Zip error: Unable to open %1 in %2").arg(QString::fromStdString(filename), zip->getZipName()));
			}
			return file.readAll();
		};

		return sol::table::create_with(state.L,
									   "read", sol::as_function([readBinary](const std::string &filename) { return QString::fromUtf8(readBinary(filename)).toStdString(); }),
									   "readb", sol::as_function(readBinary));
	};
	auto readBzip2 = [](const std::string &filename, const std::string &path)
	{
		QFile file(QString::fromStdString(filename));
		if (!file.open(QFile::ReadOnly))
		{
			throw Exception(QStringLiteral("Unable top open BZip2 file %1: %2").arg(file.fileName(), file.errorString()));
		}
		int bzerror;
		FILE *f = fdopen(file.handle(), "r");
		BZFILE *bz = BZ2_bzReadOpen(&bzerror, f, 0, 0, NULL, 0);
		if (bzerror != BZ_OK)
		{
			BZ2_bzReadClose(&bzerror, bz);
			fclose(f);
			throw Exception(QStringLiteral("Unable to open BZip2 file %1").arg(file.fileName()));
		}
		QByteArray data;
		char buf[4096];
		while (bzerror == BZ_OK)
		{
			int actualSize = BZ2_bzRead(&bzerror, bz, buf, 4096);
			data.append(QByteArray::fromRawData(buf, actualSize));
		}
		if (bzerror != BZ_STREAM_END)
		{
			BZ2_bzReadClose(&bzerror, bz);
			fclose(f);
			throw Exception(QStringLiteral("Unable to read BZip2 file %1").arg(file.fileName()));
		}
		BZ2_bzReadClose(&bzerror, bz);
		fclose(f);
		return data.toStdString();
	};

	sol::state *lua = m_script->m_lua;
	sol::table ctxt = m_script->staticContext();
	ctxt["status"] = lua->create_table_with(
				"message", sol::as_function([this](const std::string &msg) { qDebug() << "Lua task status:" << msg.c_str(); setStatus(QString::fromStdString(msg)); }),
				"progress", sol::as_function([this](const uint64_t current, const uint64_t total) { setProgress(current, total); }),
				"progress_idle", sol::as_function([this]() { setProgress(0, 0); }),
				"fail", sol::as_function([this](const std::string &msg) { throw Exception(QString::fromStdString(msg)); })
			);
	ctxt["http"] = lua->create_table_with(
				"get_file", sol::as_function(getFile),
				"get_file_to", sol::as_function(getFileAndCopy),
				"get", sol::as_function(get)
			);
	ctxt["libraries"] = lua->create_table_with(
				"from", sol::as_function(libraryFrom)
				);
	ctxt["verify_onesix_library_format"] = sol::as_function(verifyOneSixLibraryFormat);
	ctxt["verify_onesix_version_format"] = sol::as_function(verifyOneSixVersionFormat);
	ctxt["zip"] = sol::as_function(openZip);
	ctxt["bzip2"] = sol::as_function(readBzip2);
	ctxt["fs"] = lua->create_table_with();
	ctxt["from_json"] = sol::as_function([lua](const std::string &json) { return LuaUtil::fromJson(*lua, json); });
	ctxt["to_json"] = sol::as_function(&LuaUtil::toJson);

	return ctxt;
}

sol::table Script::staticContext()
{
	return m_lua->create_table_with(
				"debug", sol::as_function(&solDebug),
				"regex_one", sol::as_function(&solRegexOne),
				"regex_many", sol::as_function(&solRegexMany)
				);
}
