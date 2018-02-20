# adds a custom path and local path to the inserted CUSTOM_PATHS_VAR list which
# can be given to the GENERATE_QRC command

function(ADD_CUSTOM_QRC_PATH CUSTOM_PATHS_VAR IMPORT_PATH LOCAL_PATH)
  list(APPEND ${CUSTOM_PATHS_VAR} "${IMPORT_PATH}=${LOCAL_PATH}")
  set(${CUSTOM_PATHS_VAR} ${${CUSTOM_PATHS_VAR}} PARENT_SCOPE)
endfunction()
