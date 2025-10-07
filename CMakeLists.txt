cmake_minimum_required(VERSION 3.10)
project(TetrisGame)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 11)

# Указываем местоположение заголовочных файлов GLFW и OpenGL
include_directories(include)

# Указываем местоположение исходных файлов
set(SOURCE_FILES src/main.cpp)

# Добавляем библиотеки для компиляции
find_package(OpenGL REQUIRED)
find_package(GLFW3 REQUIRED)

add_executable(TetrisGame ${SOURCE_FILES})

# Линкуем необходимые библиотеки
target_link_libraries(TetrisGame OpenGL::GLFW3 OpenGL::GL)
