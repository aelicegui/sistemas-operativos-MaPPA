#include "../include/recursos.h"

t_list *recursos_del_sistema;

void crear_lista_recursos(){
    char **recursos_aux = recursos_config();
    char **instancias_aux = instancias_recursos_config();
    recursos_del_sistema = list_create();

    for(int i = 0; i < (string_array_size(recursos_aux)); i++){
        t_recurso *nuevo_recurso = malloc(sizeof(t_recurso));
        //nuevo_recurso->nombre = malloc(sizeof(char*));

        nuevo_recurso->nombre = strdup(recursos_aux[i]);
        nuevo_recurso->cantidad = atoi(instancias_aux[i]);
        //nuevo_recurso->disponibles = nuevo_recurso->cantidad;
        nuevo_recurso->id_recurso_lista = i;
        list_add(recursos_del_sistema, nuevo_recurso);

    }
}

t_recurso *solicitar_recurso(char *recurso_recibido){   

    int cant_recursos =  list_size(recursos_del_sistema);
    
    for (int i = 0; i < cant_recursos; i++){
        t_recurso *rec_aux = list_get(recursos_del_sistema, i);

        if(strcmp(rec_aux->nombre,recurso_recibido)== 0){
            return rec_aux;
        }
    } 

    return NULL;
}

void wait_recurso(t_pcb *pcb, char *recurso_recibido){
    
    t_recurso *recurso = solicitar_recurso(recurso_recibido);

    if(recurso == NULL){
        
        sacar_de_execute();

        //cambiar_estado_pcb(pcb, EXIT);

        agregar_a_exit(pcb, INVALID_RESOURCE);
        return;
    }
    disminuir_cantidad_recurso(pcb, recurso);

    if(recurso->cantidad < 0){
        sacar_de_execute();
        agregar_a_blocked(pcb, "recurso", recurso->nombre);
        cambiar_estado_pcb(pcb, BLOCKED);
        log_info(logger_kernel, "PID: %d - Bloqueado por: %s", pcb->contexto_ejecucion->pid, recurso_recibido);
        return;
    }



    list_add(pcb->recursos_asignados, recurso);
    
}

void signal_recurso(t_pcb *pcb, char *recurso_recibido){
    
    t_recurso *recurso = solicitar_recurso(recurso_recibido);

    if(recurso == NULL){
        sacar_de_execute();
        //cambiar_estado_pcb(pcb, EXIT);
        agregar_a_exit(pcb, INVALID_RESOURCE);
        return;
    }


    if(recursos_cargados(pcb->recursos_asignados, recurso_recibido)){

        list_remove_element(pcb->recursos_asignados, recurso);

        aumentar_cantidad_recursos(pcb, recurso);

        t_cola_block* cola_bloqueado = buscar_cola_block("recurso", recurso_recibido);
        if(queue_size(cola_bloqueado->cola_bloqueados) > 0){
            t_pcb *pcb_vuelve_ready = sacar_de_blocked("recurso", recurso_recibido); //a la primera cola que encuentre bloqueada del mismo recurso la saca de ahi
            
            agregar_a_ready(pcb_vuelve_ready); //REVISAR
        }
    }
    else{
        log_info(logger_kernel, "PID: %d - Intento hacer Signal del recurso %s pero no tenia tomada ninguna instancia del mismo", pcb->contexto_ejecucion->pid, recurso_recibido);
    }
     
}

void disminuir_cantidad_recurso(t_pcb  *pcb, t_recurso *recurso){
    
    for(int i = 0; i < list_size(recursos_del_sistema); i++){
        t_recurso *recurso_aux2 = list_get(recursos_del_sistema, i);
        if(strcmp(recurso_aux2->nombre,recurso->nombre) == 0 )
        {
            recurso_aux2->cantidad--;
            //t_recurso  *caca = list_replace(recursos_del_sistema,i,recurso_aux2); //nos cambia en la lista, y nos retorna una caquita :)
            log_info(logger_kernel, "PID: %d - Wait: %s- Instancias: %d", pcb->contexto_ejecucion->pid, recurso->nombre, recurso_aux2->cantidad);
            //free(caca);
            return;
        }
    }
}

void aumentar_cantidad_recursos(t_pcb  *pcb, t_recurso *recurso){
    
    for(int i = 0; i < list_size(recursos_del_sistema); i++){
        t_recurso *recurso_aux2 = list_get(recursos_del_sistema, i);

        if(strcmp(recurso_aux2->nombre,recurso->nombre) == 0 )
        {
            recurso_aux2->cantidad++;
            //t_recurso  *caca = list_replace(recursos_del_sistema,i,recurso_aux2); //nos cambia en la lista, y nos retorna una caquita :)
            log_info(logger_kernel, "PID: %d - Signal: %s- Instancias: %d", pcb->contexto_ejecucion->pid, recurso->nombre, recurso_aux2->cantidad);
            //free(caca);
            return;
        }
    }
}

bool recursos_cargados(t_list* recursos_asignados, char *recurso_recibido){
    t_recurso *recurso;

    for(int i=0; i < list_size(recursos_asignados); i++){
        recurso = list_get(recursos_asignados, i);
        if(strcmp(recurso->nombre, recurso_recibido) == 0){
            return true;
        }
    }
    return false;
}

void eliminar_lista_recursos(){

    list_destroy_and_destroy_elements(recursos_del_sistema, (void*)eliminar_recursos);

}

void eliminar_recursos(t_list* recursos_totales){

    for(int i = 0; i < list_size(recursos_totales) - 1; i++){
        free(list_get(recursos_totales, i));
    }
    list_destroy(recursos_totales);
}

