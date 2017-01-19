update_url = ""
version = "0.0.1"
name = "LiteLoader"
author = "MultiMC authors"
id = "org.multimc.liteloader"

url = "http://dl.liteloader.com/versions/versions.json"

register_entity_provider({
	id = "liteloader.com",
	static_entities = {
		{ id = "liteloader", name = "LiteLoader" }
	},
	version_list_factory = function(id)
		return {
			roles = {
				parent_game_version = "mcVersion",
				recommended = "recommended",
				latest = "latest"
			},
			load = function(ctxt)
				local data = ctxt.from_json(ctxt.http.get(url))

				local description = data.meta.description
				local defaultUrl = data.meta.url
				local authors = data.meta.authors

				local versions = {}

				for mcVersion, versionObj in pairs(data.versions) do
					function process_artefacts(artefacts, notSnapshots)
						local latestVersion = nil
						local vers = {}
						for identifier, artefact in pairs(artefacts) do
							if identifier == "latest" then
								latestVersion = artefact.version
							else
								local version = {
									type = (notSnapshots and "Release" or "Snapshot"),
									name = artefact.version,
									descriptor = artefact.version,

									version = artefact.version,
									mcVersion = mcVersion,
									md5 = artefact.md5,
									timestamp = tonumber(artefact.timestamp),
									tweakClass = artefact.tweakClass,
									authors = authors,
									description = description,
									defaultUrl = defaultUrl,
									isSnapshot = not notSnapshots,
									libraries = artefact.libraries,
									latest = false,
									recommended = false
								}

								for index, lib in ipairs(version.libraries) do
									if lib.name == "org.ow2.asm:asm-all:5.0.3" then
										lib.url = "http://repo.maven.apache.org/maven2/"
									end
									version.libraries[index] = lib
								end

								local mainLib = {
									name = "com.mumfrey:liteloader:" .. artefact.version,
									url = "http://dl.liteloader.com/versions/"
								}
								if not notSnapshots then
									mainLib.hint = "always-stale"
								end
								table.insert(version.libraries, mainLib)
								table.insert(vers, version)
							end
						end
						return vers, latestVersion
					end

					local releases = {}
					local latestRelease = nil
					local snapshots = {}
					local latestSnapshot = nil

					if versionObj.artefacts ~= nil then
						releases, latestRelease = process_artefacts(versionObj.artefacts["com.mumfrey:liteloader"], true)
					end
					if versionObj.snapshots ~= nil then
						snapshots, latestSnapshot = process_artefacts(versionObj.snapshots["com.mumfrey:liteloader"], false)
					end

					if latestSnapshot then
						for index, snapshot in ipairs(snapshots) do
							if snapshot.version == latestSnapshot then
								snapshots[index].latest = true
								snapshots[index].descriptor = "Latest"
							end
						end
					elseif latestRelease then
						for index, release in ipairs(releases) do
							if release.version == latestRelease then
								releases[index].latest = true
								releases[index].descriptor = "Latest"
							end
						end
					end

					if latestRelease then
						for index, release in ipairs(releases) do
							if release.version == latestRelease then
								releases[index].recommended = true
							end
						end
					end

					for index, release in ipairs(releases) do
						table.insert(versions, release)
					end
					for index, snapshot in ipairs(snapshots) do
						table.insert(versions, snapshot)
					end
				end

				return versions
			end,
			comparator = function(ctxt, a, b)
				return a.timestamp < b.timestamp
			end,
			install = function(ctxt, version)
				local data = {
					order = 10,
					mainClass = "net.minecraft.launchwrapper.Launch"
				}
				data["+tweakers"] = {version.tweakClass}
				data["+libraries"] = version.libraries
				ctxt.debug("Writing patch...")
				ctxt.write_patch(data)
				ctxt.reload()
			end
		}
	end
})
