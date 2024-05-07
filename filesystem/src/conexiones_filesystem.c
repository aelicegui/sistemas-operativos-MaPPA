#include "../include/conexiones_filesystem.h"

void* conectar_con_kernel(int *socket_kernel){
    codigo_operacion codigo;
    char* nombre_archivo;
    int dir_fisica;
    int tamanio;
    int bloque_puntero;
    int bloque;
    int tam_char;
    //void* contenido_pagina;
    //void* contenido_bloque;
    void* contenido_pagina = (void*)malloc(tam_bloque());
    void* contenido_bloque = (void*)malloc(tam_bloque());
    void* bloque_leido;
    //t_FCB* fcb; no se usaba aparentemente
    t_paquete *paquete;
    int desplazamiento;
    t_buffer *buffer_memoria;

    while(true){
        codigo = recibir_operacion(*socket_kernel);
        t_buffer *buffer = crear_buffer();
        buffer->stream = recibir_buffer(&(buffer->size), *socket_kernel);
        desplazamiento = 0;
        switch(codigo){
            case APERTURA_FS:
                memcpy(&tam_char,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                nombre_archivo = malloc(tam_char);
                memcpy(nombre_archivo,buffer->stream + desplazamiento, tam_char);
                desplazamiento+=tam_char;

                tamanio = abrir_archivo(nombre_archivo);
                paquete = crear_paquete(APERTURA_FS);
                agregar_a_paquete(paquete, &tamanio, sizeof(int));
                enviar_paquete(paquete, *socket_kernel);
                free(nombre_archivo);
                break;

            case LEER_FS:
                memcpy(&tam_char,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                nombre_archivo = malloc(tam_char);
                memcpy(nombre_archivo,buffer->stream + desplazamiento, tam_char);
                desplazamiento+=tam_char;
                memcpy(&dir_fisica,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                memcpy(&bloque_puntero,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                bloque = buscar_bloque_archivo((bloque_puntero/tam_bloque()), nombre_archivo);
                acceder_bloque_archivo(bloque, nombre_archivo);
                bloque_leido = leer_bloque(bloque);
                //enviar a memoria
                memcpy(contenido_bloque, bloque_leido, tam_bloque());
                paquete = crear_paquete(F_READ);
                agregar_a_paquete(paquete, contenido_bloque, tam_bloque());
                agregar_a_paquete(paquete, &dir_fisica, tam_bloque());
                enviar_paquete(paquete, conexion_memoria);

                codigo = recibir_operacion(conexion_memoria);
                if(codigo == OK){
                    codigo = recibir_operacion(conexion_memoria);
                }
                if(codigo == F_READ){
                    paquete = crear_paquete(LEER_FS);
                    enviar_paquete(paquete, *socket_kernel);
                    log_leer_archivo(nombre_archivo, bloque_puntero, dir_fisica);
                    free(nombre_archivo);
                }
                break;

            case ESCRIBIR_FS:
                memcpy(&tam_char,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                nombre_archivo = malloc(tam_char);
                memcpy(nombre_archivo,buffer->stream + desplazamiento, tam_char);
                desplazamiento+=tam_char;
                memcpy(&dir_fisica,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                memcpy(&bloque_puntero,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                bloque = buscar_bloque_archivo((bloque_puntero/tam_bloque()), nombre_archivo);
                paquete = crear_paquete(F_WRITE);
                agregar_a_paquete(paquete, &dir_fisica, sizeof(int));
                enviar_paquete(paquete, conexion_memoria);

                codigo = recibir_operacion(conexion_memoria);
                if(codigo == OK){
                    codigo = recibir_operacion(conexion_memoria);
                }
                if(codigo == F_WRITE){
                    buffer_memoria = crear_buffer();
                    buffer_memoria->stream = recibir_buffer(&(buffer_memoria->size), conexion_memoria);
                    int desplazamiento_memoria = 0;

                    memcpy(contenido_pagina,buffer_memoria->stream + desplazamiento_memoria, tam_bloque());
                    desplazamiento_memoria+=tam_bloque();

                    escribir_bloque(contenido_pagina, bloque);
                    paquete = crear_paquete(ESCRIBIR_FS);
                    enviar_paquete(paquete, *socket_kernel);
                    log_escribir_archivo(nombre_archivo, bloque_puntero, dir_fisica);
                    liberar_buffer(buffer_memoria);
                }
                free(nombre_archivo);
                break;

            case TRUNCATE_FS:
                memcpy(&tam_char,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                nombre_archivo = malloc(tam_char);
                memcpy(nombre_archivo,buffer->stream + desplazamiento, tam_char);
                desplazamiento+=tam_char;
                memcpy(&tamanio,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento+=sizeof(int);
                truncar_fcb(nombre_archivo, tamanio);
                paquete = crear_paquete(TRUNCATE_FS);
                enviar_paquete(paquete,*socket_kernel);
                //free(nombre_archivo);
                break;

            default:
                break;
        }
        liberar_buffer(buffer); 
    }
    free(contenido_pagina);
    free(contenido_bloque);
}


void* conectar_con_memoria(int *socket_memoria){
    codigo_operacion codigo;
    int cantidad_bloques;
    int* bloques_asignados;
    int* bloques_a_liberar;
    //void* contenido_pagina;
    void* contenido_pagina = (void*)malloc(tam_bloque());
    void* contenido_bloque;
    int posicion_swap;
    int bloque;
    int desplazamiento;
    t_paquete *paquete;
    while(true){
        codigo = recibir_operacion(*socket_memoria);
        t_buffer *buffer = crear_buffer();
        buffer->stream = recibir_buffer(&(buffer->size), *socket_memoria);
        desplazamiento = 0;
        switch(codigo){
            case INICIAR_PROCESO_FS:
                memcpy(&cantidad_bloques,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento += sizeof(int);
                bloques_asignados = asignar_bloques_SWAP(cantidad_bloques);
                paquete = crear_paquete(BLOQUE_SWAP);
                agregar_a_paquete(paquete,bloques_asignados, cantidad_bloques*sizeof(int));
                enviar_paquete(paquete,*socket_memoria);
                free(bloques_asignados);
                break;
            case FINALIZAR_PROCESO_FS:
                memcpy(&cantidad_bloques,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento += sizeof(int);
                bloques_a_liberar = malloc(cantidad_bloques*sizeof(int));
                memcpy(bloques_a_liberar,buffer->stream + desplazamiento, cantidad_bloques*sizeof(int));
                desplazamiento += cantidad_bloques*sizeof(int);
                liberar_bloques_SWAP(bloques_a_liberar, cantidad_bloques); //ANDA PARA LA MIERDA REVISAR
                free(bloques_a_liberar);
                break;
            case ESCRIBIR_SWAP:
                memcpy(contenido_pagina,buffer->stream + desplazamiento, tam_bloque());
                desplazamiento += tam_bloque();
                memcpy(&posicion_swap,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento += sizeof(int);
                bloque = posicion_swap / tam_bloque();

                escribir_bloque(contenido_pagina, bloque);
                
                paquete = crear_paquete(OK);
                enviar_paquete(paquete,*socket_memoria);
                log_acceso_Bloque_SWAP(bloque);
                break;
            case LEER_SWAP:
                memcpy(&posicion_swap,buffer->stream + desplazamiento, sizeof(int));
                desplazamiento += sizeof(int);
                bloque = posicion_swap / tam_bloque();

                contenido_bloque = leer_bloque(bloque);

                paquete = crear_paquete(BLOQUE_SWAP);
                agregar_a_paquete(paquete, contenido_bloque, tam_bloque());
                enviar_paquete(paquete,*socket_memoria);
                log_acceso_Bloque_SWAP(bloque);
                  
                break;
            default:
                break;
        }
        liberar_buffer(buffer);  
    }
}
