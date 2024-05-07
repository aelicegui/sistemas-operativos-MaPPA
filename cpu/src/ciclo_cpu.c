#include "../include/ciclo_cpu.h"

int hayInterrupcion;
sem_t recibir_instruccion;
sem_t mutex_ctx;
t_contexto_ejecucion* ctx_global;
void comenzar_ciclo_instruccion(t_contexto_ejecucion* ctx)
{
    ctx_global = ctx;
    sem_init(&recibir_instruccion,0,0);
    sem_init(&mutex_ctx,0,1);
    t_instruccion *instruccion_a_ejecutar;
    hayInterrupcion = 0;
    pthread_t hilo_interrupciones;
    pthread_create(&hilo_interrupciones, NULL, (void*)check_interrupt, ctx);//hilo que recibe interrupciones del KERNEL
    while(ctx_global != NULL && !hayInterrupcion)//Corroborar otras cosas creo!!!!! ctx->program_counter <= ctx->cant_instrucciones
    {   
    
        instruccion_a_ejecutar = fetch(ctx_global); //ASIGNE MEMORIA PARA LA ISNTRUCCION VERIFICAR QUE SE LIBERE
        if(!decode(instruccion_a_ejecutar))
        {
            ctx_global->program_counter ++;
        }
        
        codigo_operacion operacion = execute(instruccion_a_ejecutar, ctx_global);

        switch (operacion)
        {
        //DEVOLVER EL CTX SIN PARAMETROS
        case EXIT_OP: 
            sem_wait(&mutex_ctx);
            if(ctx_global != NULL){
                devolver_contexto(ctx_global,operacion);
                liberar_contexto(ctx_global);
                ctx_global = NULL;
            }
            sem_post(&mutex_ctx);
            break;

        //DEVOLVER CTX CON UN PARAMETRO 
        case SIGNAL: 
        case SLEEP:
        case WAIT:
        case F_CLOSE:
            sem_wait(&mutex_ctx);
            if(ctx_global != NULL){
                devolver_contexto_con_parametro(ctx_global,operacion, instruccion_a_ejecutar->parametros[0]);
                liberar_contexto(ctx_global);
                ctx_global = NULL;
            }
            sem_post(&mutex_ctx);
            break;

        //DEVOLVER CTX CON DOS PARAMETROS
        case F_OPEN: 
        case F_SEEK:
        case F_TRUNCATE:
        
            sem_wait(&mutex_ctx);
            if(ctx_global != NULL)
            {
                devolver_contexto_con_parametros(ctx_global,operacion,instruccion_a_ejecutar->parametros[0],instruccion_a_ejecutar->parametros[1]);
                liberar_contexto(ctx_global);
                ctx_global = NULL;
            }
            sem_post(&mutex_ctx);
            break;

        //DEVOLVER POR PAGE FAULT --> CTX Y NUMERO DE PAGINA
        case MOV_OUT:
            sem_wait(&mutex_ctx);
            int num_pag = numero_pagina(atoi(instruccion_a_ejecutar->parametros[0]));
            devolver_contexto_page_fault(ctx_global,num_pag);
            liberar_contexto(ctx_global);
            ctx_global = NULL;
            sem_post(&mutex_ctx);
            break;
        
        case MOV_IN: case F_READ: case F_WRITE:
            sem_wait(&mutex_ctx);
            int num_pagina = numero_pagina(atoi(instruccion_a_ejecutar->parametros[1]));
            devolver_contexto_page_fault(ctx_global,num_pagina);
            liberar_contexto(ctx_global);
            ctx_global = NULL;
            sem_post(&mutex_ctx);
            break;

        //NO HAY QUE DEVOLVER EL CTX 
        case NADA: 
            break;
        default :
            break;
        }
        
       
    }

    if(hayInterrupcion){
       sem_post(&recibir_instruccion);
       pthread_join(hilo_interrupciones, NULL);
    }
    else{
        pthread_cancel(hilo_interrupciones);
    }
    
}

t_instruccion *fetch(t_contexto_ejecucion *ctx)
{
    log_info(logger_cpu,"PID: %d - FETCH - Program Counter: %d.", ctx->pid, ctx->program_counter);

    t_instruccion *instruccion = solicitar_instruccion_a_memoria(ctx->program_counter, ctx -> pid);
    
    return instruccion;
}

bool decode(t_instruccion *instruccion)
{
    codigo_operacion operacion = instruccion->operacion;

    return (operacion == MOV_IN || operacion == MOV_OUT || operacion == F_READ || operacion == F_WRITE);
}

bool es_numero_valido(const char *str)
{
    while (*str)
    {
        if (!isdigit(*str))
            return false;
        str++;
    }
    return true;
}

int execute(t_instruccion *instruccion, t_contexto_ejecucion *ctx)//creo que aca deberia liberar la instruccion
{   
    int dir_fisica;
    uint32_t *registro;
    switch (instruccion->operacion)
    {
    case SET://SET(Registro,Valor)
        
        log_info(logger_cpu, "PID: %d - Ejecutando: SET - %s %s." , ctx->pid, instruccion->parametros[0], instruccion->parametros[1]);
        asignar_a_registro(instruccion->parametros[0], instruccion->parametros[1], ctx->registros); //VER COMO SE ENVIAN BIEN LOS PUNTEROS
        return NADA;

    case SUM://SUM(Registro Destino,Registro Origen)
    
        log_info(logger_cpu, "PID: %d - Ejecutando: SUM - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        operar_registros(instruccion->operacion,instruccion->parametros[0], instruccion->parametros[1], ctx->registros);
        return NADA;
        
    case SUB:

        log_info(logger_cpu, "PID: %d - Ejecutando: SUB - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        operar_registros(instruccion->operacion,instruccion->parametros[0], instruccion->parametros[1], ctx->registros);
        return NADA;

    case JNZ://JNZ(Registro,Instruccion)

        log_info(logger_cpu, "PID: %d - Ejecutando: JNZ - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        registro = obtener_registro(instruccion->parametros[0], ctx->registros);
        if(*registro != 0 && es_numero_valido(instruccion->parametros[1]))
        {   
            ctx->program_counter = atoi(instruccion->parametros[1]);
        }
        return NADA;//CREO 
    
    case SIGNAL:

        log_info(logger_cpu, "PID: %d - Ejecutando: SIGNAL - %s." , ctx->pid, instruccion->parametros[0]);   
        return SIGNAL;
    
    case SLEEP:

        log_info(logger_cpu, "PID: %d - Ejecutando: SLEEP - %s." , ctx->pid, instruccion->parametros[0]);
        return SLEEP;

    case WAIT:

        log_info(logger_cpu, "PID: %d - Ejecutando: WAIT - %s." , ctx->pid, instruccion->parametros[0]);
        return WAIT;

    case EXIT_OP:

        log_info(logger_cpu, "PID: %d - Ejecutando: EXIT." , ctx->pid);
        return EXIT_OP;

    case MOV_IN:// MOV_IN (Registro, Dirección Lógica)

        log_info(logger_cpu, "PID: %d - Ejecutando: MOV_IN - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        dir_fisica = traducir_direccion_mmu(atoi(instruccion->parametros[1]), ctx);
        if(dir_fisica == -1)
        {
            return MOV_IN;
        }
        char *valor_leido = leer_de_memoria(dir_fisica,ctx->pid);
        asignar_a_registro(instruccion->parametros[0], valor_leido, ctx->registros);
        log_info(logger_cpu, "PID: %d - Acción: LEER - Dirección Física: %d - Valor: %s", ctx->pid, dir_fisica, valor_leido);
        free(valor_leido);
        return NADA;
    
    case MOV_OUT://MOV_OUT (Dirección Lógica, Registro)
        log_info(logger_cpu, "PID: %d - Ejecutando: MOV_OUT - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        dir_fisica = traducir_direccion_mmu(atoi(instruccion->parametros[0]), ctx);
        if(dir_fisica == -1)
        {   
            log_info(cpu, "HUBO PAGE FAULT");
            return MOV_OUT;
        }
        
        registro =  obtener_registro(instruccion->parametros[1], ctx->registros);
        int num_pagina = numero_pagina(atoi(instruccion->parametros[0]));
        escribir_en_memoria(dir_fisica,ctx->pid,*registro, num_pagina);
        log_info(logger_cpu, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d", ctx->pid, dir_fisica, *registro);
        return NADA;

    case F_OPEN://F_OPEN(Nombre Archivo, Modo Apertura)

        log_info(logger_cpu, "PID: %d - Ejecutando: F_OPEN - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        return F_OPEN;
    
    case F_CLOSE://F_CLOSE(Nombre Archivo)

        log_info(logger_cpu, "PID: %d - Ejecutando: F_CLOSE - %s." , ctx->pid, instruccion->parametros[0]);
        return F_CLOSE;
    
    case F_SEEK://F_SEEK(Nombre Archivo, Posicion)

        log_info(logger_cpu, "PID: %d - Ejecutando: F_SEEK - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        return F_SEEK;
    
    case F_READ:

        log_info(logger_cpu, "PID: %d - Ejecutando: F_READ - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        dir_fisica = traducir_direccion_mmu(atoi(instruccion->parametros[1]), ctx);
        if(dir_fisica == -1)
        {
            return F_READ;
        }
        
            if(ctx_global != NULL)
            {
                devolver_contexto_con_int(ctx,F_READ,instruccion->parametros[0],dir_fisica);
                liberar_contexto(ctx);
                ctx_global = NULL;
            }
        
        return NADA;
    
    case F_WRITE: 

        log_info(logger_cpu, "PID: %d - Ejecutando: F_WRITE - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        dir_fisica = traducir_direccion_mmu(atoi(instruccion->parametros[1]), ctx);
        if(dir_fisica == -1)
        {
            return F_WRITE;
        }
            if(ctx_global != NULL)
            {
                devolver_contexto_con_int(ctx,F_WRITE,instruccion->parametros[0],dir_fisica);
                liberar_contexto(ctx);
                ctx_global = NULL;
            }

       return NADA;
    
    case F_TRUNCATE://F_TRUNCATE(Nombre Archivo, Tamanio)

        log_info(logger_cpu, "PID: %d - Ejecutando: F_TRUNCATE - %s %s." , ctx->pid, instruccion->parametros[0],instruccion->parametros[1]);
        return F_TRUNCATE;
    
    default:
        return 0;
    }

}

void check_interrupt(t_contexto_ejecucion *ctx)
{
    codigo_operacion interrupcion_motivo;
    int PID;
    
    recv(cliente_kernel_interrupt, &interrupcion_motivo, sizeof(codigo_operacion),MSG_WAITALL);//Recibo el motivo de interrupcion
    recv(cliente_kernel_interrupt, &PID, sizeof(int),MSG_WAITALL);
    
    hayInterrupcion = 1;
    sem_wait(&recibir_instruccion);
    sem_wait(&mutex_ctx);
    if(ctx_global != NULL && ctx_global->pid == PID)
    {
        enviar_contexto(ctx_global,cliente_kernel_dispatch,interrupcion_motivo); //REVISAR
        liberar_contexto(ctx_global);
        ctx_global = NULL;
       
    }
    sem_post(&mutex_ctx);
}

int traducir_direccion_mmu(int dir_logica, t_contexto_ejecucion *ctx)
{
    int desplazamiento = dir_logica - numero_pagina(dir_logica) * tam_pag;
   
    int num_marco = solicitar_numero_de_marco(numero_pagina(dir_logica), ctx->pid);
    if(num_marco == -1)
    {
        return -1;
    }
    
    int dir_fisica = (num_marco * tam_pag) + desplazamiento;
    ctx->program_counter++;
    return dir_fisica;
}

int numero_pagina(int dir_logica)
{
    return floor(dir_logica/ tam_pag);
}