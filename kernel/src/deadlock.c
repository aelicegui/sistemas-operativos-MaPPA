#include "../include/deadlock.h"

void *procesos_en_blocked(){

    while(1){

        sem_wait(&hayPCBsEnBlocked);

        log_info(logger_kernel, "ANALISIS DE DETECCION DE DEADLOCKS");

        if(!detectar_deadlock()){

            log_info(kernel, "No se genera deadlock entre los procesos bloqueados");

        }else{
            
            log_info(kernel, "Finalizar el proceso correspondiente que elimine el deadlock");
        
        }
    }
}

bool detectar_deadlock(){

    //lista auxiliar con las colas de procesos bloqueados
    t_list *colas_procesos_bloqueados_aux = list_create();
    list_add_all(colas_procesos_bloqueados_aux, lista_block); 

    //variables de largo para recorrer las colas bloqueadas y sus procesos adjuntos
    int cant_colas_bloqueadas_aux = list_size(colas_procesos_bloqueados_aux);
    int cant_proc_cola_bloqueada;

    //primera lista para guardar los pcbs en retencion
    t_list *pcbs_con_retencion = list_create();

    //variables para recorrer colas
    int pid;
    t_pcb *pcb;

    //contador de elementos (pueden ser recursos o archivos) de un pcb
    int cantidad_de_elementos = 0;

    //-----------------------------PRIMER PASO: ANALIZAR LA RETENCION--------------------------------------------

    for(int i = 0; i < cant_colas_bloqueadas_aux ; i++){
        
        sem_wait(&block_sem);
        t_cola_block *cola_a_analizar = list_get(colas_procesos_bloqueados_aux,i);
        sem_post(&block_sem);

        cant_proc_cola_bloqueada = queue_size(cola_a_analizar->cola_bloqueados);

        for(int j = 0; j < cant_proc_cola_bloqueada; j++){
        
            pid = pid_bloqueado(cola_a_analizar);

            pcb = buscarPcb(pid);
            
            cantidad_de_elementos = list_size(pcb->recursos_asignados) + list_size(pcb->tabla_archivos_abiertos);

            if(cantidad_de_elementos > 0){
                t_pcb_bloqueado *pcb_bloqueado_elem = malloc(sizeof(t_pcb_bloqueado)); 

                pcb_bloqueado_elem->pid = pid;
                pcb_bloqueado_elem->elemento_bloqueante = cola_a_analizar->identificador;
                pcb_bloqueado_elem->elementos_asignados = list_create();

                agregar_elementos_asignados(pcb_bloqueado_elem, pcb); //carga al pcb bloqueado con lo que tiene el pcb

                list_add(pcbs_con_retencion, pcb_bloqueado_elem); 
            }
            cantidad_de_elementos = 0;
        }
    }

    //variables auxiliares (se usan a lo largo de los dos pasos)
    t_pcb_bloqueado *pcb_bloqueado;
    t_pcb_bloqueado *pcb_bloqueado_auxiliar;

    //variables contadores
    int cadena;
    int cant_pcbs_con_retencion = list_size(pcbs_con_retencion);

    //segunda lista de cadenas, que cada cadena que contiene pcbs bloqueados
    t_list *cadenas_con_espera_circular = list_create();

    //-----------------------------SEGUNDO PASO PARTE 1: ESPERA CIRCULAR--------------------------------------------
    
    for(int i = 0; i < cant_pcbs_con_retencion ; i++){
        
        pcb_bloqueado = list_get(pcbs_con_retencion, i);
    
        for(int j = i+1; j < cant_pcbs_con_retencion; j++){
            
            //agarras a los siguientes de la misma lista
            pcb_bloqueado_auxiliar = list_get(pcbs_con_retencion, j);
            
            //es una lista de pcbs bloqueados
            t_list *cadena_posible_deadlock;
            
            //analizo el primer camino
            if(list_contains(pcb_bloqueado_auxiliar->elementos_asignados, pcb_bloqueado->elemento_bloqueante)){
                
                cadena = cadena_a_continuar_cadena(pcb_bloqueado, cadenas_con_espera_circular);

                if(cadena == -1){ //caso inicial
                    cadena_posible_deadlock = list_create();
                    list_add(cadena_posible_deadlock, pcb_bloqueado);
                    list_add(cadena_posible_deadlock, pcb_bloqueado_auxiliar);
                    list_add(cadenas_con_espera_circular, cadena_posible_deadlock);
                }
                else{
                    cadena_posible_deadlock = list_get(cadenas_con_espera_circular, cadena);
                    list_add(cadena_posible_deadlock, pcb_bloqueado_auxiliar); //en este caso agrego el que no tengo en la cadena
                }
            }
            
            //analizo el segundo camino (el primero al reves)
            if(list_contains(pcb_bloqueado->elementos_asignados, pcb_bloqueado_auxiliar->elemento_bloqueante)){
                
                //posible caso que rompa: te va a decir que no tiene pero que ya existe una cadena
                cadena = cadena_a_continuar_cadena(pcb_bloqueado_auxiliar, cadenas_con_espera_circular);
                
                if(cadena == -1){ //caso inicial
                    cadena_posible_deadlock = list_create();
                    list_add(cadena_posible_deadlock, pcb_bloqueado_auxiliar);
                    list_add(cadena_posible_deadlock, pcb_bloqueado);
                    list_add(cadenas_con_espera_circular, cadena_posible_deadlock);
                }
                else{
                    cadena_posible_deadlock = list_get(cadenas_con_espera_circular, cadena);
                    list_add(cadena_posible_deadlock, pcb_bloqueado); //en este caso agrego el que no tengo en la cadena
                }

            }
            
        }
    }
    //-----------------------------SEGUNDO PASO PARTE 2: CERRAR LAS CADENAS--------------------------------------------
    
    //lista auxiliar
    t_list *cadena_auxiliar;
    
    //contadores
    int cantidad_cadenas = list_size(cadenas_con_espera_circular);
    int cantidad_pcbs;

    for(int w = 0; w < cantidad_cadenas; w++){

        cadena_auxiliar = list_get(cadenas_con_espera_circular,w);

        cantidad_pcbs = list_size(cadena_auxiliar);

        pcb_bloqueado = list_get(cadena_auxiliar,0); //aca guardamos un pcb

        pcb_bloqueado_auxiliar = list_get(cadena_auxiliar,(cantidad_pcbs-1)); //aca guardamos otro pcb

        //analizamos si las cadenas cierran
        if(list_contains(pcb_bloqueado->elementos_asignados, pcb_bloqueado_auxiliar->elemento_bloqueante)){
            for(int k = 0; k < cantidad_pcbs; k++){
                pcb_bloqueado_auxiliar = list_get(cadena_auxiliar,k);
                list_add(pcbs_en_deadlock,pcb_bloqueado_auxiliar);
            }
        }
    }

    //-----------------------------MOSTRAR EN CONSOLA EL LOG OBLIGATORIO--------------------------------------------
    int tamanio_pcbs_deadlock = list_size(pcbs_en_deadlock);
    t_pcb_bloqueado *pcb_deadlock;
    char *elementos_en_posicion;
    
    for(int i = 0; i < tamanio_pcbs_deadlock; i++){

        pcb_deadlock = list_get(pcbs_en_deadlock, i);

        elementos_en_posicion = string_new();
        string_append(&(elementos_en_posicion), list_get(pcb_deadlock->elementos_asignados, 0));
        for(int j = 1; j < list_size(pcb_deadlock->elementos_asignados); j++){
            string_append(&(elementos_en_posicion), ", ");
            string_append(&(elementos_en_posicion), list_get(pcb_deadlock->elementos_asignados, j));
        }
        
        log_info(logger_kernel, "Deadlock detectado: %d - Recursos en posesión: %s - Recurso requerido: %s", pcb_deadlock->pid, elementos_en_posicion, pcb_deadlock->elemento_bloqueante);

    }

    list_destroy(colas_procesos_bloqueados_aux);
    list_destroy(cadenas_con_espera_circular);
    libera_listas(pcbs_con_retencion);

    return (list_size(pcbs_en_deadlock) > 0);

}

bool list_contains(t_list* lista, char* nombre_elemento){
    int elementos_lista = list_size(lista);
    char* auxiliar;

    for(int i=0; i<elementos_lista;i++){
        auxiliar = list_get(lista, i);
        if(strcmp(auxiliar, nombre_elemento)==0){
            return true;
        }
    }
    return false;
}

int contar_apariciones(t_list* lista, int pid){
    int elementos_lista = list_size(lista);
    int auxiliar;
    int cantidad=0;

    for(int i=0; i<elementos_lista;i++){
        auxiliar = atoi(list_get(lista, i));
        if(auxiliar == pid){
            cantidad++;
        }
    }
    return cantidad;
}

int cadena_a_continuar_cadena(t_pcb_bloqueado* pcb_bloqueado, t_list* lista){
    t_list* auxiliar;
    int tam_lista;
    t_pcb_bloqueado* aux_pcb_bloqueado;
    for(int i=0; i<list_size(lista);i++){
        auxiliar = list_get(lista, i);
        tam_lista = list_size(auxiliar);
        aux_pcb_bloqueado = list_get(auxiliar, (tam_lista-1));
        if(aux_pcb_bloqueado->pid == pcb_bloqueado->pid){
            return i;
        }
    }
    
    return -1;

}

void sin_repetidos(t_list* lista){
    int cantidad;
    int vueltas = list_size(lista);
    
    for(int i = (vueltas-1);i >= 0; i--){
        cantidad = contar_apariciones(lista, atoi(list_get(lista, i)));
        if(cantidad > 1){
            list_remove(lista, i);
        }
    }


}

void agregar_elementos_asignados(t_pcb_bloqueado* pcb_bloqueado, t_pcb* pcb){

    int cant_recursos = list_size(pcb->recursos_asignados);
    int cant_archivos = list_size(pcb->tabla_archivos_abiertos);

    t_recurso* recurso_aux;
    char* nombre_recurso;

    t_archivos_proceso* archivo_aux;
    char* nombre_archivo;

    for(int i=0; i<cant_recursos; i++){
        recurso_aux = list_get(pcb->recursos_asignados,i);
        nombre_recurso = recurso_aux->nombre;
        list_add(pcb_bloqueado->elementos_asignados, nombre_recurso);
    }
    for(int j=0; j<cant_archivos; j++){
        archivo_aux = list_get(pcb->tabla_archivos_abiertos,j);
        nombre_archivo = archivo_aux->fcb->nombre;
        list_add(pcb_bloqueado->elementos_asignados, nombre_archivo);
    }
}

void *liberar_pcb_block(t_pcb_bloqueado* pcb_bloqueado){

    list_destroy(pcb_bloqueado->elementos_asignados);

    free(pcb_bloqueado);

    return NULL;
}

void libera_listas(t_list *lista_a_liberar){

    for(int i = 0; i < (list_size(lista_a_liberar)); i++){
        
        t_pcb_bloqueado *pcb_bloqueado_aux = list_get(lista_a_liberar, i);
        liberar_pcb_block(pcb_bloqueado_aux);
    }

    list_destroy(lista_a_liberar);

}