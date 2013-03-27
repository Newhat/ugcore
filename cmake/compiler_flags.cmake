# This function sets the CMAKE_C_FLAGS and CMAKE_CXX_FLAGS variabels.
# 
# TODO: maybe add sanity checks for flags
#
function (add_c_flag flag)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" CACHE STRING "overriden flags!" FORCE)
endfunction(add_c_flag)

function (add_cpp_flag flag)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" CACHE STRING "overriden flags!" FORCE)
endfunction(add_cpp_flag)

function(add_cxx_flag flag)
	add_c_flag(${flag})
	add_cpp_flag(${flag})
endfunction(add_cxx_flag)

################################################################################
# Used to clear c/cxx flags. Handles environment variables CXX_FLAGS and C_FLAGS
function(reset_cxx_flags)
	# check env for set flags, and overwrite present cxx flag string
	set(CXX_FLAGS "$ENV{CXX_FLAGS}" CACHE STRING "custom cxx flags" FORCE)
	set(C_FLAGS "$ENV{C_FLAGS}" CACHE STRING "custom c flags" FORCE)
	
	# first reset flags, then check env for given flags
	foreach(lang C CXX)
		set(CMAKE_${lang}_FLAGS "" CACHE STRING "clear flags" FORCE)
	endforeach()
	
	# re-add custom flags for c and c++
	if(CXX_FLAGS)
		add_cpp_flag(${CXX_FLAGS})
	endif()
	
	if(C_FLAGS)
		add_c_flag(${CXX_FLAGS})
	endif()
endfunction(reset_cxx_flags)
