#include "../include/consola.h"

int grado_multi;
//mensaje : INICIAR_PROCESO sdadasd 23 1
void iniciarProceso(char *mensaje){

    char* path = strtok(mensaje," ");

    mensaje+= strlen(path)+1;

    char* size = strtok(mensaje," ");
    //log_info(kernel, "%s", size);

    mensaje+= strlen(size)+1;

    char *prioridad = strtok(mensaje," ");

    mensaje+= strlen(prioridad)+1;

    int size_path = atoi(size);

    int prioridade = atoi(prioridad);

    crear_pcb(path, size_path, prioridade);
    

}


void finalizarProcesoPID(char *mensaje){

    char* pid_char = strtok(mensaje, " ");

    int pid = atoi(pid_char);

    t_pcb *pcb_a_finalizar;

    pcb_a_finalizar = buscarPcb(pid);

    //manejo de memoria: eliminacion de procesos (condicion por si estaba en exec)

    if(pcb_en_execute != NULL && pcb_a_finalizar->contexto_ejecucion->pid == pcb_en_execute->contexto_ejecucion->pid)
    {
        enviar_interrupcion(CONSOLA);
        send(conexion_cpu_interrupt, &(pcb_a_finalizar->contexto_ejecucion->pid), sizeof(int), 0);
    }
    else{
        sacar_pcb_de_lista(pcb_a_finalizar);
        agregar_a_exit(pcb_a_finalizar, CONSOLA);
    }

    
    
}

void detenerPlanificacion(){
    sem_wait(&cortoPlazoHablilitado);
    sem_wait(&largoPlazoHabilitado);

    log_info(logger_kernel,"PAUSA DE PLANIFICACION");

}

void iniciarPlanificacion(){
    

    sem_post(&cortoPlazoHablilitado);
    sem_post(&largoPlazoHabilitado);

    log_info(logger_kernel,"INICIO DE PLANIFICACION");
    
}

void modificarGradoMultiprogramacion(char *mensaje) {

    char* gradoMulti = strtok(mensaje, " ");

    int nuevo_grado = atoi(gradoMulti);

    sem_destroy(&grado_multiprogramacion_sem);
    sem_init(&grado_multiprogramacion_sem, 0, nuevo_grado);

    log_info(logger_kernel,"Grado Anterior: %d - Grado Actual: %d ", grado_multiprogramacion(), nuevo_grado); 



}
/*
void modificarGradoMultiprogramacion(char *mensaje) {

    char* gradoMulti = strtok(mensaje, " ");

    int nuevo_grado = atoi(gradoMulti);

    int diff = grado_multi - nuevo_grado;

    //esto de aca te lo permite modificar en tiempo real
    if(diff > 0){
        for(int i = 0; i < diff; i++){ //con este for podemos hacer el cambio en tiempo real (esto es para que podamos hacer bien la prueba integral)
            sem_wait(&grado_multiprogramacion_sem);
        }
    }
    else{
        for(int i = diff; i<0; i++){
            sem_post(&grado_multiprogramacion_sem);
        }
    }
    
    log_info(logger_kernel,"Grado Anterior: %d - Grado Actual: %d ", grado_multi, nuevo_grado); 

    grado_multi = nuevo_grado; //aca se actualiza el grado


}*/

void sacar_pcb_de_lista(t_pcb* pcb){
    int cant_colas_bloqueadas;
    int cant_proc_cola_bloqueada;

    switch(pcb->estado){
        case READY:
            sacar_de_lista(lista_ready, pcb->contexto_ejecucion->pid);
            sem_wait(&hayPCBsEnReady);
            break;
        case NEW:
            sacar_de_lista(lista_new, pcb->contexto_ejecucion->pid);
            sem_wait(&hayPCBsEnNew);
            break;
        case BLOCKED:
            cant_colas_bloqueadas = list_size(lista_block);
            for(int i = 0; i < cant_colas_bloqueadas ; i++){
                t_cola_block *cola_a_analizar = list_get(lista_block,i);
                cant_proc_cola_bloqueada = queue_size(cola_a_analizar->cola_bloqueados);
                for(int j = 0; j < cant_proc_cola_bloqueada; j++){
                    int* pid_bloqueado = (int *)queue_pop(cola_a_analizar->cola_bloqueados);
                    if(pcb->contexto_ejecucion->pid == *pid_bloqueado){
                        i=cant_colas_bloqueadas;
                        break;
                    }
                    queue_push(cola_a_analizar->cola_bloqueados, pid_bloqueado);
                }
            }
            break;
        default:
            exit(EXIT_FAILURE);
            break;
    }

    
}

void sacar_de_lista(t_list * lista, int pid){
    int elementos_lista = list_size(lista);
    t_pcb* pcb;
    for(int i=0; i<elementos_lista; i++){
        pcb = list_get(lista, i);
        if(pcb->contexto_ejecucion->pid == pid){
            list_remove(lista, i);
            return;
        }
    }
    exit(EXIT_FAILURE);
}


void listarProcesosPorEstado() {
    
    char *lista_pids_new = listar_pids(lista_new);
    log_info(logger_kernel,"Estado: NEW - Procesos:  [%s]", lista_pids_new);

    char *lista_pids_ready = listar_pids(lista_ready);
    log_info(logger_kernel,"Estado: READY - Procesos:  [%s]", lista_pids_ready);

    char *lista_pids_exit = listar_pids(lista_exit);
    log_info(logger_kernel,"Estado: EXIT - Procesos:  [%s]", lista_pids_exit);

    char *lista_pids_exec = listar_pids(lista_exec);
    log_info(logger_kernel,"Estado: EXEC - Procesos:  [%s]", lista_pids_exec);

    char *lista_pids_blocked = listar_pids_block(lista_block);
    log_info(logger_kernel,"Estado: BLOCKED - Procesos:  [%s]", lista_pids_blocked);
}


void consola(){
    grado_multi = grado_multiprogramacion();

    while (1) {

        t_mensajes_consola mensaje_consola;
        char input[50];
        char *mensaje;
        char *puntero = input;
        printf("\033[0;32m");
        printf(">- ");
        printf("\033[0m");
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0'; //no estaba entrando en ninguno de los casos por el enter que hacer un barra n
        mensaje = strtok(puntero, " ");

        mensaje_consola = mensaje_a_consola(mensaje);

        switch(mensaje_consola){

            case INICIAR_PROCESO:
                puntero+= strlen(mensaje)+1;
                iniciarProceso(puntero);

                break;


            case FINALIZAR_PROCESO:
                puntero+= strlen(mensaje)+1;
                finalizarProcesoPID(puntero);

                break;

            case INICIAR_PLANIFICACION:

                iniciarPlanificacion();

                break;

            case DETENER_PLANIFICACION:

                detenerPlanificacion();


                break;

            case MULTIPROGRAMACION:

                puntero+= strlen(mensaje)+1;
                modificarGradoMultiprogramacion(puntero);

                break;

            case PROCESO_ESTADO:

                listarProcesosPorEstado();

                break;

            default:
            
                break;

        }
        
    }
}

t_mensajes_consola mensaje_a_consola(char *mensaje_consola){
    if(strcmp(mensaje_consola,"INICIAR_PROCESO") == 0){
        return INICIAR_PROCESO;
    }
    if(strcmp(mensaje_consola,"FINALIZAR_PROCESO") == 0){
        return FINALIZAR_PROCESO;
    }
    if(strcmp(mensaje_consola,"DETENER_PLANIFICACION") == 0){
        return DETENER_PLANIFICACION;
    }
    if(strcmp(mensaje_consola,"INICIAR_PLANIFICACION") == 0){
        return INICIAR_PLANIFICACION;
    }
    if(strcmp(mensaje_consola,"MULTIPROGRAMACION") == 0){
        return MULTIPROGRAMACION;
    }
    if(strcmp(mensaje_consola,"PROCESO_ESTADO") == 0){
        return PROCESO_ESTADO;
    }else
        return ERROR;
}