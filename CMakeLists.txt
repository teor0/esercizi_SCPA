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

add_executable(int_broadcast_version
    src/calcolo_integrale/broadcast_version.c
)

add_executable(int_derived_version
    src/calcolo_integrale/derived_version.c
)

add_executable(best_version
    src/calcolo_integrale/best_version.c
)

add_executable(int_scatter_version
    src/calcolo_integrale/scatter_version.c
)

add_executable(dp_scatter_version
    src/dot_product/scatter_version.c
)

target_link_libraries(hello PRIVATE MPI::MPI_C)

target_link_libraries(pingpong PRIVATE MPI::MPI_C)

target_link_libraries(calcolo_integrale PRIVATE MPI::MPI_C)

target_link_libraries(calcolo_integrale_root PRIVATE MPI::MPI_C)

target_link_libraries(int_broadcast_version PRIVATE MPI::MPI_C)

target_link_libraries(best_version PRIVATE MPI::MPI_C)

target_link_libraries(int_derived_version PRIVATE MPI::MPI_C)

target_link_libraries(int_scatter_version PRIVATE MPI::MPI_C)

target_link_libraries(dp_scatter_version PRIVATE MPI::MPI_C)
