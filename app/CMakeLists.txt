cmake_minimum_required(VERSION 3.4.1)

add_library( native-lib
             SHARED
             src/main/cpp/native-lib.cpp
             )
add_library( Echo
        SHARED
        src/main/cpp/echo/Echo.cpp
        )

find_library( log-lib
              log )

target_link_libraries( native-lib
                        Echo
                       ${log-lib} )