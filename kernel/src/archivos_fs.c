#include "../include/archivos_fs.h"


//-------------------------------
//Funciones de Manejo Filesystem
//-------------------------------

//F_OPEN

bool existe_lock(char *nombre_archivo_open){

    t_archivos_proceso *archivo = buscar_archivo_en_tabla_archivos(nombre_archivo_open);

    return archivo != NULL;
}

locks lock(char *nombre_archivo_open){
    
    t_archivos_proceso *archivo = buscar_archivo_en_tabla_archivos(nombre_archivo_open);
    if(archivo != NULL){
        return archivo->tipo_lock;
    }
    else{
        return W;//no ta weno pero creo funca
    }
    
}

//F_SEEK
int actualizar_puntero_archivo(char *nombre_archivo_fseek, int posicion){
    
    t_archivos_proceso *archivo = buscar_archivo_abierto(pcb_en_execute->tabla_archivos_abiertos, nombre_archivo_fseek);

    if(archivo == NULL){
        return -1;
    }else{
        archivo->puntero_bloque = posicion;
        return 1;
    }
}

t_archivos_proceso *buscar_archivo_en_tabla_archivos(char *nombre_archivo_fseek){
    
    for(int i = 0; i < list_size(tabla_global_archivos_abiertos); i++){

        t_archivos_proceso *archivo = list_get(tabla_global_archivos_abiertos, i);

        if(strcmp(nombre_archivo_fseek, archivo->fcb->nombre) == 0){
            return archivo;
        }
        
    }
    return NULL;
}

//F_TRUNCATE
int truncar_archivo(char *nombre_archivo_read, int tamanio){

    t_paquete *paquete = crear_paquete(TRUNCATE_FS);
    //enviar paquete a fs con estos datos para que trunque el archivo
    int tam_char = strlen(nombre_archivo_read)+1;
    agregar_a_paquete(paquete, &tam_char, sizeof(int));
    agregar_a_paquete(paquete, nombre_archivo_read, tam_char);
    agregar_a_paquete(paquete,&tamanio, sizeof(int));
    agregar_a_paquete(paquete, &(pcb_en_execute->contexto_ejecucion->pid), sizeof(int));
    enviar_paquete(paquete,conexion_a_filesystem);

    //IMPLEMENTAR EL BLOQUEO PROCESO
    codigo_operacion cod_op = recibir_operacion(conexion_a_filesystem);
    if(cod_op == OK){
        cod_op = recibir_operacion(conexion_a_filesystem);
    }
    if(cod_op != TRUNCATE_FS){
        return -1;
    }else{
        return 1;
    }
}

void cerrar_archivo(t_pcb* pcb, char* nombre_archivo_close){
    t_archivos_proceso* archivo = buscar_archivo_abierto(pcb->tabla_archivos_abiertos, nombre_archivo_close);
    list_remove_element(pcb->tabla_archivos_abiertos, archivo);
    t_archivos_proceso* archivo_global = buscar_archivo_abierto(tabla_global_archivos_abiertos, nombre_archivo_close);
    if(archivo->tipo_lock == R){
        archivo_global->participantes_lock--;
        free(archivo->fcb);
    }
    if(archivo_global->participantes_lock == 0){
        list_remove_element(tabla_global_archivos_abiertos, archivo_global);
        free(archivo_global->fcb);
        free(archivo_global);

        t_cola_block* archivo_abierto = buscar_cola_del_archivo_abierto(nombre_archivo_close);
        if(archivo_abierto != NULL){
            while(!queue_is_empty(archivo_abierto->cola_bloqueados)){
                t_pcb *pcb = sacar_de_blocked("archivo", nombre_archivo_close);
                agregar_a_ready(pcb);
            }
            borrar_cola_block(archivo_abierto->identificador);
            queue_destroy(archivo_abierto->cola_bloqueados);
            free(archivo_abierto);
            
        }
        
    }
    free(archivo);
}

void borrar_cola_block(char *nombre){
    t_cola_block* cola_recurso;
    int cantidad_colas = list_size(lista_block);

    for(int i=0; i<cantidad_colas; i++){
        cola_recurso = list_get(lista_block, i);
        if(strcmp(cola_recurso->tipo, "archivo") == 0){
            if(strcmp(cola_recurso->identificador, nombre) == 0){
                list_remove(lista_block, i);
            }
        }
    }
}