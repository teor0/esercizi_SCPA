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

add_executable(pingpong
    src/pingpong/main.c
)

add_executable(calcolo_integrale
    src/calcolo_integrale/main.c
)

add_executable(calcolo_integrale_root
    src/calcolo_integrale/root_version.c
)

target_link_libraries(hello PRIVATE MPI::MPI_C)

target_link_libraries(pingpong PRIVATE MPI::MPI_C)

target_link_libraries(calcolo_integrale PRIVATE MPI::MPI_C)

target_link_libraries(calcolo_integrale_root PRIVATE MPI::MPI_C)
