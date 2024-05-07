# Libraries
LIBS=shared commons pthread

# Custom libraries' paths
SHARED_LIBPATHS=
STATIC_LIBPATHS=../shared

# Compiler flags
CDEBUG=-g -Wall -DDEBUG -fcommon -fdiagnostics-color=always
CRELEASE=-O3 -Wall -DNDEBUG -fcommon

# Arguments when executing with start, memcheck or helgrind
ARGS=kernel.config
ARGS_BASE=pruebas/base/prueba_base.config
ARGS_DEADLOCK=pruebas/deadlock/prueba_deadlock.config
ARGS_ERRORES=pruebas/errores/prueba_errores.config
ARGS_ESTRES=pruebas/estres/prueba_estres.config
ARGS_FS=pruebas/fs/prueba_fs.config
ARGS_INTEGRAL=pruebas/integral/prueba_integral.config
ARGS_MEMORIA=pruebas/memoria/prueba_memoria.config
ARGS_RECURSOS=pruebas/recursos/prueba_recursos.config

# Valgrind flags
MEMCHECK_FLAGS=--track-origins=yes --log-file="memcheck.log"
HELGRIND_FLAGS=

# Source files (*.c) to be excluded from tests compilation
TEST_EXCLUDE=src/kernel.c
