#include "LaunchProfile.h"
#include <Version.h>

void LaunchProfile::clear()
{
	m_minecraftVersion.clear();
	m_minecraftVersionType.clear();
	m_minecraftAssets.reset();
	m_minecraftArguments.clear();
	m_tweakers.clear();
	m_mainClass.clear();
	m_appletClass.clear();
	m_libraries.clear();
	m_traits.clear();
	m_jarMods.clear();
	m_mainJar.reset();
	m_problemSeverity = ProblemSeverity::None;
}

static void applyString(const QString & from, QString & to)
{
	if(from.isEmpty())
		return;
	to = from;
}

void LaunchProfile::applyMinecraftVersion(const QString& id)
{
	applyString(id, this->m_minecraftVersion);
}

void LaunchProfile::applyAppletClass(const QString& appletClass)
{
	applyString(appletClass, this->m_appletClass);
}

void LaunchProfile::applyMainClass(const QString& mainClass)
{
	applyString(mainClass, this->m_mainClass);
}

void LaunchProfile::applyMinecraftArguments(const QString& minecraftArguments)
{
	applyString(minecraftArguments, this->m_minecraftArguments);
}

void LaunchProfile::applyMinecraftVersionType(const QString& type)
{
	applyString(type, this->m_minecraftVersionType);
}

void LaunchProfile::applyMinecraftAssets(MojangAssetIndexInfo::Ptr assets)
{
	if(assets)
	{
		m_minecraftAssets = assets;
	}
}

void LaunchProfile::applyTraits(const QSet<QString>& traits)
{
	this->m_traits.unite(traits);
}

void LaunchProfile::applyTweakers(const QStringList& tweakers)
{
	// if the applied tweakers override an existing one, skip it. this effectively moves it later in the sequence
	QStringList newTweakers;
	for(auto & tweaker: m_tweakers)
	{
		if (tweakers.contains(tweaker))
		{
			continue;
		}
		newTweakers.append(tweaker);
	}
	// then just append the new tweakers (or moved original ones)
	newTweakers += tweakers;
	m_tweakers = newTweakers;
}

void LaunchProfile::applyJarMods(const QList<LibraryPtr>& jarMods)
{
	this->m_jarMods.append(jarMods);
}

static int findLibraryByName(QList<LibraryPtr> *haystack, const GradleSpecifier &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack->size(); ++i)
	{
		if (haystack->at(i)->rawName().matchName(needle))
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

void LaunchProfile::applyMods(const QList<LibraryPtr>& mods)
{
	QList<LibraryPtr> * list = &m_mods;
	for(auto & mod: mods)
	{
		auto modCopy = Library::limitedCopy(mod);

		// find the mod by name.
		const int index = findLibraryByName(list, mod->rawName());
		// mod not found? just add it.
		if (index < 0)
		{
			list->append(modCopy);
			return;
		}

		auto existingLibrary = list->at(index);
		// if we are higher it means we should update
		if (Version(mod->version()) > Version(existingLibrary->version()))
		{
			list->replace(index, modCopy);
		}
	}
}

void LaunchProfile::applyLibrary(LibraryPtr library)
{
	if(!library->isActive())
	{
		return;
	}

	QList<LibraryPtr> * list = &m_libraries;
	if(library->isNative())
	{
		list = &m_nativeLibraries;
	}

	auto libraryCopy = Library::limitedCopy(library);

	// find the library by name.
	const int index = findLibraryByName(list, library->rawName());
	// library not found? just add it.
	if (index < 0)
	{
		list->append(libraryCopy);
		return;
	}

	auto existingLibrary = list->at(index);
	// if we are higher it means we should update
	if (Version(library->version()) > Version(existingLibrary->version()))
	{
		list->replace(index, libraryCopy);
	}
}

void LaunchProfile::applyMainJar(LibraryPtr jar)
{
	if(jar)
	{
		m_mainJar = jar;
	}
}

void LaunchProfile::applyProblemSeverity(ProblemSeverity severity)
{
	if (m_problemSeverity < severity)
	{
		m_problemSeverity = severity;
	}
}

const LibraryPtr LaunchProfile::getMainJar() const
{
	return m_mainJar;
}

QString LaunchProfile::getMinecraftVersion() const
{
	return m_minecraftVersion;
}

QString LaunchProfile::getAppletClass() const
{
	return m_appletClass;
}

QString LaunchProfile::getMainClass() const
{
	return m_mainClass;
}

const QSet<QString> &LaunchProfile::getTraits() const
{
	return m_traits;
}

const QStringList & LaunchProfile::getTweakers() const
{
	return m_tweakers;
}

bool LaunchProfile::hasTrait(const QString& trait) const
{
	return m_traits.contains(trait);
}

ProblemSeverity LaunchProfile::getProblemSeverity() const
{
	return m_problemSeverity;
}

QString LaunchProfile::getMinecraftVersionType() const
{
	return m_minecraftVersionType;
}

std::shared_ptr<MojangAssetIndexInfo> LaunchProfile::getMinecraftAssets() const
{
	if(!m_minecraftAssets)
	{
		return std::make_shared<MojangAssetIndexInfo>("legacy");
	}
	return m_minecraftAssets;
}

QString LaunchProfile::getMinecraftArguments() const
{
	return m_minecraftArguments;
}

const QList<LibraryPtr> & LaunchProfile::getJarMods() const
{
	return m_jarMods;
}

const QList<LibraryPtr> & LaunchProfile::getLibraries() const
{
	return m_libraries;
}

const QList<LibraryPtr> & LaunchProfile::getNativeLibraries() const
{
	return m_nativeLibraries;
}

void LaunchProfile::getLibraryFiles(
	const QString& architecture,
	QStringList& jars,
	QStringList& nativeJars,
	const QString& overridePath,
	const QString& tempPath
) const
{
	QStringList native32, native64;
	jars.clear();
	nativeJars.clear();
	for (auto lib : getLibraries())
	{
		lib->getApplicableFiles(currentSystem, jars, nativeJars, native32, native64, overridePath);
	}
	// NOTE: order is important here, add main jar last to the lists
	if(m_mainJar)
	{
		// FIXME: HACK!! jar modding is weird and unsystematic!
		if(m_jarMods.size())
		{
			QDir tempDir(tempPath);
			jars.append(tempDir.absoluteFilePath("minecraft.jar"));
		}
		else
		{
			m_mainJar->getApplicableFiles(currentSystem, jars, nativeJars, native32, native64, overridePath);
		}
	}
	for (auto lib : getNativeLibraries())
	{
		lib->getApplicableFiles(currentSystem, jars, nativeJars, native32, native64, overridePath);
	}
	if(architecture == "32")
	{
		nativeJars.append(native32);
	}
	else if(architecture == "64")
	{
		nativeJars.append(native64);
	}
}

QStringList LaunchProfile::getClassPath(
	const QString& architecture,
	const QString& localPath,
	const QString& binRoot
) const
{
	QStringList jars, nativeJars;
	getLibraryFiles(architecture, jars, nativeJars, localPath, binRoot);
	return jars;
}

QStringList LaunchProfile::getNativeJars(
	const QString& architecture,
	const QString& localPath,
	const QString& binRoot
) const
{
	QStringList jars, nativeJars;
	getLibraryFiles(architecture, jars, nativeJars, localPath, binRoot);
	return nativeJars;
}

QStringList LaunchProfile::verboseDescription(AuthSessionPtr session)
{
	QStringList out;
	out << "Main Class:" << "  " + getMainClass() << "";
	out << "Native path:" << "  " + getNativePath() << "";


	auto alltraits = getTraits();
	if(alltraits.size())
	{
		out << "Traits:";
		for (auto trait : alltraits)
		{
			out << "traits " + trait;
		}
		out << "";
	}

	// libraries and class path.
	{
		out << "Libraries:";
		QStringList jars, nativeJars;
		auto javaArchitecture = settings()->get("JavaArchitecture").toString();
		getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
		auto printLibFile = [&](const QString & path)
		{
			QFileInfo info(path);
			if(info.exists())
			{
				out << "  " + path;
			}
			else
			{
				out << "  " + path + " (missing)";
			}
		};
		for(auto file: jars)
		{
			printLibFile(file);
		}
		out << "";
		out << "Native libraries:";
		for(auto file: nativeJars)
		{
			printLibFile(file);
		}
		out << "";
	}

	if(loaderModList()->size())
	{
		out << "Mods:";
		for(auto & mod: loaderModList()->allMods())
		{
			if(!mod.enabled())
				continue;
			if(mod.type() == Mod::MOD_FOLDER)
				continue;
			// TODO: proper implementation would need to descend into folders.

			out << "  " + mod.filename().completeBaseName();
		}
		out << "";
	}

	if(coreModList()->size())
	{
		out << "Core Mods:";
		for(auto & coremod: coreModList()->allMods())
		{
			if(!coremod.enabled())
				continue;
			if(coremod.type() == Mod::MOD_FOLDER)
				continue;
			// TODO: proper implementation would need to descend into folders.

			out << "  " + coremod.filename().completeBaseName();
		}
		out << "";
	}

	auto & jarMods = getJarMods();
	if(jarMods.size())
	{
		out << "Jar Mods:";
		for(auto & jarmod: jarMods)
		{
			auto displayname = jarmod->displayName(currentSystem);
			auto realname = jarmod->filename(currentSystem);
			if(displayname != realname)
			{
				out << "  " + displayname + " (" + realname + ")";
			}
			else
			{
				out << "  " + realname;
			}
		}
		out << "";
	}

	auto params = processMinecraftArgs(nullptr);
	out << "Params:";
	out << "  " + params.join(' ');
	out << "";

	QString windowParams;
	if (settings()->get("LaunchMaximized").toBool())
	{
		out << "Window size: max (if available)";
	}
	else
	{
		auto width = settings()->get("MinecraftWinWidth").toInt();
		auto height = settings()->get("MinecraftWinHeight").toInt();
		out << "Window size: " + QString::number(width) + " x " + QString::number(height);
	}
	out << "";
	return out;
}
