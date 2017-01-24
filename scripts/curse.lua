update_url = ""
version = "0.0.1"
name = "Curse"
author = "MultiMC authors"
id = "org.multimc.curse"

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

			local id = string.format("%s-%d", entity.Name:gsub("[^%a%d]+", ""):gsub(" ", "-"):lower(), entity.Id)
			authors = table.concat(authors, ", ")
			table.insert(entities, {
				id = id,
				name = entity.Name,
				icon_url = icon_url,
				author = authors
			})
		end
		return entities
	end,
	version_list_factory = function(id)
	end
})
