cmake_minimum_required(VERSION 3.16)

project(esercizi LANGUAGES C)

find_package(MPI REQUIRED COMPONENTS C)

# per aggiungere un altro esercizio alla compilazione segui il pattern
# add_executable(esempio
#     src/altro/esempio.c
# )
# target_link_libraries(esempio PRIVATE MPI::MPI_C)


add_executable(hello
    src/helloworld/hello.c
)


target_link_libraries(hello PRIVATE MPI::MPI_C)

target_compile_options(hello PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
