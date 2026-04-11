#This is for Mac OS
cmake_minimum_required(VERSION 3.10)
project(HttpdWin)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the executable name and its source files
add_executable(HttpdWin 
    cookie.cpp 
    httpdlog.cpp
    httprequest.cpp
    httpconnection.cpp
    httpheaders.cpp
    httpresponse.cpp
    threadpool.cpp
    main.cpp
)
target_compile_definitions(HttpdWin PUBLIC -DMAC_TAHOE)

target_include_directories(HttpdWin PRIVATE ./)
target_include_directories(HttpdWin PUBLIC /opt/local/include)
target_include_directories(HttpdWin PUBLIC /opt/local/include/openssl-3)
target_include_directories(HttpdWin PUBLIC /Library/Frameworks/Python.framework/Versions/3.14)
target_include_directories(HttpdWin PUBLIC /Library/Frameworks/Python.framework/Versions/3.14/include)
target_include_directories(HttpdWin PUBLIC /Library/Frameworks/Python.framework/Versions/3.14/include/python3.14)

target_link_directories(HttpdWin PUBLIC /opt/local/lib/openssl-3/)
target_link_directories(HttpdWin PUBLIC /Library/Frameworks/Python.framework/Versions/3.14/) 
target_link_directories(HttpdWin PUBLIC /Library/Frameworks/Python.framework/Versions/3.14/lib)

target_link_libraries(HttpdWin -lpython3.14 -lcrypto -lssl)





