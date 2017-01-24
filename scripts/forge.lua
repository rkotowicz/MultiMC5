update_url = ""
version = "0.0.1"
name = "Minecraft Forge"
author = "MultiMC authors"
id = "org.multimc.forge"

function forge_uses_installer(mcversion)
	return mcversion ~= "1.5.2"
end

function string:split(sep)
		local sep, fields = sep or ":", {}
		local pattern = string.format("([^%s]+)", sep)
		self:gsub(pattern, function(c) fields[#fields+1] = c end)
		return fields
end
function string.starts(str, start)
	return string.sub(str, 1, string.len(start)) == start
end

register_entity_provider({
	id = "net.minecraftforge",
	static_entities = {
		{
			id = "net.minecraftforge",
			name = "Minecraft Forge",
			icon_url = "https://cdn.rawgit.com/MinecraftForge/MinecraftForge/5da0ac73b936185ed4f3cc2f35c6228d93e6a69d/icon.ico",
			author = "Forge developers"
		}
	},
	version_list_factory = function(id)
		return {
			roles = {
				parent_game_version = "mcVersion",
				recommended = "recommended",
				latest = "latest"
			},
			load = function(ctxt)
				legacy_versions = function()
					json = ctxt.from_json(ctxt.http.get("https://files.minecraftforge.net/minecraftforge/json"))
				end
				gradle_versions = function()
					json = ctxt.from_json(ctxt.http.get("https://files.minecraftforge.net/maven/net/minecraftforge/forge/json", {no_refetch = true}))

					local webpath = json.webpath:gsub("http:", "https:")
					local artifact = json.artifact

					local versions_map = {}
					for number, verjson in pairs(json.number) do
						if verjson.mcversion ~= nil then
							local version = {
								name = verjson.version,
								type = nil,
								descriptor = verjson.build,

								branch = verjson.branch,
								mcVersionRaw = verjson.mcversion,
								mcVersion = verjson.mcversion:gsub("_pre", "-pre"),

								universal_file = nil,
								installer_file = nil,
								changelog_url = nil,

								recommended = false,
								latest = false
							}

							for index, file in ipairs(verjson.files) do
								local extension = file[1]
								local part = file[2]
								local checksum = file[3]

								local long_version = version.mcVersionRaw .. "-" .. version.name
								if version.branch ~= nil then
									long_version = long_version .. "-" .. version.branch
								end
								local filename = artifact .. "-" .. long_version .. "-" .. part .. "." .. extension
								local url = webpath .. "/" .. long_version .. "/" .. filename

								if part == "installer" then
									version.installer_file = {
										filename = filename,
										url = url,
										md5 = checksum
									}
								elseif part == "universal" or part == "client" then
									version.universal_file = {
										filename = filename,
										url = url,
										md5 = checksum
									}
								elseif part == "changelog" then
									version.changelog_url = url
								end
							end

							if version.universal_file ~= nil or version.installer_file ~= nil then
								versions_map[number] = version
							end
						end
					end

					for type, number in pairs(json.promos) do
						local match = ctxt.regex_one("^(?<mcversion>[0-9]+(.[0-9]+)*)-(?<label>[a-z]+)$", type)
						if match == nil then
							ctxt.debug("warning: " .. type .. " doesn't match the promo regex")
						elseif versions_map[number] ~= nil then
							local label = match.captured("label")
							local mcver = match.captured("mcversion")
							if label == "recommended" then
								versions_map[number].recommended = true
							elseif label == "latest" then
								versions_map[number].latest = true
							end
						end
					end

					local versions = {}
					for number, version in pairs(versions_map) do
						table.insert(versions, version)
					end
					return versions
				end

				return gradle_versions()
			end,
			comparator = function(ctxt, a, b)
				return a.descriptor < b.descriptor
			end,
			install = function(ctxt, version)
				ctxt.patch.remove_if_exists()

				local file = forge_uses_installer(version.mcVersion) and version.installer_file or version.universal_file
				local dl_filename = ctxt.http.get_file(file.url, { md5 = file.md5, no_refetch = true })

				if forge_uses_installer(version.mcVersion) then
					-- prepare
					ctxt.status.message("Installing from Zip...")
					local zip_file = ctxt.zip(dl_filename)
					local install_profile = ctxt.from_json(zip_file.read("install_profile.json"))
					if install_profile.install == nil or install_profile.versionInfo == nil then
						ctxt.status.fail("Missing install or versionInfo in install_profile.json")
					end
					if not ctxt.verify_onesix_version_format(install_profile.versionInfo) then
						ctxt.status.fail("Invalid version in install_profile.json")
					end

					local install = install_profile.install
					local data = zip_file.readb(install.filePath)
					ctxt.libraries.from(install.path, data)

					local versionInfo = install_profile.versionInfo

					-- add
					ctxt.status.message("Creating patch data...")

					local blacklist = {"authlib", "realms"}
					local xzlist = {"org.scala-lang", "com.typesafe"}
					local lwjgl_whitelist = {
						"net.java.jinput:jinput",
						"net.java.jinput:jinput-platform",
						"net.java.jutils:jutils",
						"org.lwjgl.lwjgl:lwjgl",
						"org.lwjgl.lwjgl:lwjgl_util",
						"org.lwjgl.lwjgl:lwjgl-platform"
					}

					local patch_data = versionInfo
					patch_data["order"] = 5
					patch_data["mcVersion"] = version.mcVersion
					patch_data["+tweakers"] = {}

					local minecraft_patch = ctxt.patch.read("net.minecraft")

					ctxt.status.message("Processing libraries...")
					local libraries = {}
					for index, library in ipairs(patch_data["libraries"]) do
						local parts = library.name:split(":")
						local prefix = parts[1] .. ":" .. parts[2]

						local found_in_minecraft = false
						for ind, lib in ipairs(minecraft_patch) do
							local mc_lib_parts = lib:split(":")
							if mc_lib_parts[1] == parts[1] and mc_lib_parts[2] == parts[2] and mc_lib_parts[3] == parts[3] then
								found_in_minecraft = true
								break
							end
						end

						-- ignore lwjgl, other blacklisted and libraries already included in minecraft
						if not has_value(lwjgl_whitelist, prefix) and not has_value(blacklist, library.name) and not found_in_minecraft then
							if parts[1] == "forge" then
								library.name = string.format("%s:%s:%s:universal", parts[1], parts[2], parts[3])
							elseif parts[1] == "minecraftforge" then
								library.name = string.format("net.minecraftforge:forge:%s-%s:universal", version.mcVersionRaw, version.name)
							end

							for ind, xz_lib in ipairs(xzlist) do
								if string.starts(library.name, xz_lib) then
									library["hint"] = "forge-pack-xz"
								end
							end

							table.insert(libraries, library)
						end
					end
					patch_data["libraries"] = libraries

					ctxt.status.message("Processing tweakers...")
					local tweaker_matches = ctxt.regex_many("--tweakClass (?<class>[a-zA-Z0-9\\.]*)", patch_data["minecraftArguments"])
					while tweaker_matches.has_next() do
						local match = tweaker_matches.next()
						table.insert(patch_data["+tweakers"], match.captured("class"))
					end

					-- reset some things we do not want to be passed along
					patch_data["releaseTime"] = nil
					patch_data["updateTime"] = nil
					patch_data["minimumLauncherVersion"] = nil
					patch_data["type"] = nil
					patch_data["minecraftArguments"] = nil
					patch_data["minecraftVersion"] = nil

					ctxt.status.message("Writing patch...")
					ctxt.patch.write(patch_data)
				else
					-- addLegacy
				end
			end
		}
	end
})
