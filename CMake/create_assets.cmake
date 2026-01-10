if(NOT DST_FILE)
	set(DST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets.cpp)
endif()
if(NOT VAR_NAME)
	set(VAR_NAME assets)
endif()
#crate file
file(WRITE ${DST_FILE} "#include <map>\n#include <string>\nnamespace ${NAMESPACE}{\nstd::map<std::string, std::string>  ${VAR_NAME} = {\n")

function(add_dir PATH ROOT)

	if(IS_DIRECTORY ${FILE})
		file(GLOB FILES ${PATH}/*)
		foreach(FILE ${FILES})
			add_dir(${FILE} ${ROOT})
		endforeach()
	else()
		#	get relative path
		file(RELATIVE_PATH ASSET_RELATIVE_PATH ${ROOT} ${PATH})
		#	#begin asset write
		file(APPEND ${DST_FILE} "{\"${ASSET_RELATIVE_PATH}\", std::string(\"")

		#Copy asset to DST_FILE
		#Read file in binary mode
		file(READ ${PATH} ASSET_CONTENTS HEX)
		#Write file in binary mode by appending to DST_FILE 2 hex characters at a time
		if(ASSET_CONTENTS)
			string(LENGTH ${ASSET_CONTENTS} ASSET_CONTENTS_LENGTH)
			math(EXPR BYTES_COUNT "${ASSET_CONTENTS_LENGTH} / 2")
			math(EXPR ASSET_CONTENTS_LENGTH "${ASSET_CONTENTS_LENGTH} - 1")


			foreach(i RANGE 0 ${ASSET_CONTENTS_LENGTH} 2)
				string(SUBSTRING ${ASSET_CONTENTS} ${i} 2 ASSET_CONTENTS_SUBSTRING)
				file(APPEND ${DST_FILE} "\\x${ASSET_CONTENTS_SUBSTRING}")
#				add "\n" every 1024 bytes
				math(EXPR MOD "${i} % 1024")
				if(MOD EQUAL 0)
					file(APPEND ${DST_FILE} "\"\n\"")
				endif()
			endforeach()
			#end asset write
			file(APPEND ${DST_FILE} "\",${BYTES_COUNT})},\n")
		else()
			file(APPEND ${DST_FILE} "\")},\n")
		endif()
	endif()

endfunction()

foreach(FILE ${ASSETS})
	#	get parent directory
	get_filename_component(PARENT_DIR ${FILE} DIRECTORY)
	add_dir(${FILE} ${PARENT_DIR})
endforeach()

#close file
file(APPEND ${DST_FILE} "};}\n")