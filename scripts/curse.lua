update_url = ""
version = "0.0.1"
name = "Curse"
author = "MultiMC authors"
id = "org.multimc.curse"

function string:split(sep)
		local sep, fields = sep or ":", {}
		local pattern = string.format("([^%s]+)", sep)
		self:gsub(pattern, function(c) fields[#fields+1] = c end)
		return fields
end

register_entity_provider({
	id = "com.curse",
	dynamic_entities = function(ctxt)
		ctxt.debug("Loading entities...")
		local raw = ctxt.bzip2(ctxt.http.get_file("http://clientupdate-v6.cursecdn.com/feed/addons/432/v10/complete.json.bz2"))
		local json = ctxt.from_json(raw)

		local entities = {}
		for index, entity in ipairs(json.data) do

			-- find default attachment
			local icon_url = nil
			if entity.Attachments ~= nil then
				for ind, attachment in ipairs(entity.Attachments) do
					if attachment.IsDefault then
						icon_url = attachment.Url
						break
					end
				end
			end

			-- collect authors
			local authors = {}
			for ind, author in ipairs(entity.Authors) do
				table.insert(authors, author.Name)
			end

			-- get and transform type
			local type = entity.CategorySection.Name
			if type == 'Mods' then
				type = EntityType.Mod
			elseif type == 'Modpacks' then
				type = EntityType.Modpack
			elseif type == 'Texture Packs' then
				type = EntityType.TexturePack
			elseif type == 'Worlds' then
				type = EntityType.World
			else
				type = EntityType.Other
			end

			local id = string.format("%s-%d", entity.Name:gsub("[^%a%d]+", ""):gsub(" ", "-"):lower(), entity.Id)
			authors = table.concat(authors, ", ")
			table.insert(entities, {
				id = id,
				name = entity.Name,
				icon_url = icon_url,
				author = authors,
				type = type,
				widget_url = string.format("https://widget.mcf.li/project/%i.json", entity.Id)
			})
		end
		return entities
	end,
	version_list_factory = function(entity)
		return {
			roles = {
				parent_game_version = "mcVersion"
			},
			load = function(ctxt)
				local data = ctxt.from_json(ctxt.http.get(entity.widget_url))

				local versions = {}
				for mcVer, mcVers in pairs(data.versions) do
					for index, ver in ipairs(mcVers) do
						local name_parts = ver.name:split("-")
						local type = nil
						if ver.id == data.download.id then
							type = data.release_type
						end
						table.insert(versions, {
							name = table.remove(name_parts),
							descriptor = string.format("%i", ver.id),
							type = type,
							timestamp = parse_iso8601(ver.created_at),

							mcVersion = mcVer,
							webpage = ver.url
						})
					end
				end

				return versions
			end,
			comparator = function(ctxt, a, b)
				return a.timestamp < b.timestamp
			end,
			install = function(ctxt, version)
				local parts = version.webpage:split("/")
				local file_id = table.remove(parts)
				local slug = entity.type == EntityType.Mod and parts[5] or parts[5]:sub(parts[5]:find("-")+1)
				local url = string.format("https://minecraft.curseforge.com/projects/%s/files/%s/download", slug, file_id)

				if entity.type == EntityType.Mod then
					ctxt.http.get_file_to(url, string.format("minecraft/mods/%s-%s-%s.jar", entity.id, version.mcVersion, version.name))
				elseif entity.type == EntityType.TexturePack then
					-- TODO: texturepacks folder
					ctxt.http.get_file_to(url, string.format("minecraft/resourcepacks/%s-%s-%s.zip", entity.id, version.mcVersion, version.name))
				elseif entity.type == EntityType.World then
					local filename = ctxt.http.get_file(url)
					ctxt.extractZip(filename, "minecraft/saves")
				elseif entity.Type == EntityType.Modpack then
					ctxt.status.fail("modpack install not yet implemented")
				end
			end
		}
	end
})
