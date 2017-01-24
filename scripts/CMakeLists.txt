set(SCRIPTS
	liteloader.lua
	forge.lua
)

foreach(script ${SCRIPTS})
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${script}
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${script}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${CMAKE_CURRENT_BINARY_DIR}/${script}
	)
	list(APPEND script_targets ${CMAKE_CURRENT_BINARY_DIR}/${script})
endforeach()
add_custom_target(copy_scripts ALL
	DEPENDS ${script_targets}
	SOURCES ${SCRIPTS}
	COMMENT "Copying scripts"
)
install(FILES ${script_targets} DESTINATION scripts)
