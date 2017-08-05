if (LIBCOMPAT_LIBRARIES AND LIBCOMPAT_INCLUDE_DIRS)
    set (LIBCOMPAT_FIND_QUIETLY TRUE)
endif (LIBCOMPAT_LIBRARIES AND LIBCOMPAT_INCLUDE_DIRS)

find_path (LIBCOMPAT_INCLUDE_DIRS NAMES libcompat.h)
find_library (LIBCOMPAT_LIBRARIES NAMES compat)

if (LIBCOMPAT_LIBRARIES)
    add_definitions(-DHAVE_LIBCOMPAT)
endif()

include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBCOMPAT DEFAULT_MSG
    LIBCOMPAT_LIBRARIES
    LIBCOMPAT_INCLUDE_DIRS)

mark_as_advanced(LIBCOMPAT_INCLUDE_DIRS LIBCOMPAT_LIBRARIES)
