cmake_minimum_required(VERSION 3.16)
project(esercizi LANGUAGES C)

find_package(MPI REQUIRED COMPONENTS C)

set(esercizi
    hello:src/helloworld/hello.c
    simple:src/helloworld/simple.c
    ordered:src/helloworld/ordered.c
    msg_exchange:src/msg_exchange/main.c
    broadcast:src/msg_exchange/broadcast.c
    somma:src/somma/main.c
    non_blocking:src/non_blocking/main.c
    pingpong:src/pingpong/main.c
    calcolo_integrale:src/calcolo_integrale/main.c
    calcolo_integrale_root:src/calcolo_integrale/root_version.c
    int_broadcast_version:src/calcolo_integrale/broadcast_version.c
    int_derived_version:src/calcolo_integrale/derived_version.c
    best_version:src/calcolo_integrale/best_version.c
    int_scatter_version:src/calcolo_integrale/scatter_version.c
    dp_scatter_version:src/dot_product/scatter_version.c
    cart_example:src/cart_topology/cart_main.c
    dt_version:src/mv_product/dt_version.c
    book_version:src/mv_product/book_version.c
    ct_version:src/mv_product/ct_version.c
)

foreach(e IN LISTS esercizi)
    string(REPLACE ":" ";" parts "${e}")
    list(GET parts 0 target)
    list(GET parts 1 source)

    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE MPI::MPI_C)
endforeach()
