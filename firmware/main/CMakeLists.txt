file(GLOB_RECURSE BORNEO_APP_C_SOURCES 
    "./src/*.c" 
)

file(GLOB_RECURSE BORNEO_APP_CPP_SOURCES 
    "./src/*.cpp" 
)

set(BORNEO_APP_INCLUDE_DIRS "./include")




idf_component_register(
    SRCS ${BORNEO_APP_C_SOURCES} ${BORNEO_APP_CPP_SOURCES}
    INCLUDE_DIRS ${BORNEO_APP_INCLUDE_DIRS}
    REQUIRES borneo
)