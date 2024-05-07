#include "../include/conexion_kernel.h"


void conectar_con_kernel(int *socket_kernel)
{   

    int pid;
    int desplazamiento;
  
    while(1)
    {
        t_buffer *buffer = crear_buffer();
        t_paquete *paquete;
        codigo_operacion cod_op = recibir_operacion(*socket_kernel);
        buffer->stream = recibir_buffer(&(buffer->size), *socket_kernel);
        desplazamiento = 0;
        int num_pag;
        int zandunga;
        switch (cod_op)
        {
        case CREAR_PROCESO:

            crear_proceso(buffer);
            send(*socket_kernel, &zandunga, sizeof(int), 0);
            break;
        
        case CARGAR_PAGINA:

            
            
            memcpy(&num_pag, buffer->stream + desplazamiento, sizeof(int));
            desplazamiento += sizeof(int);
            memcpy(&pid, buffer->stream + desplazamiento , sizeof(int));
            desplazamiento += sizeof(int);
            solicitar_pagina(num_pag, pid);
            log_info(logger_memoria,"PID: %d - Pagina: %d - Marco: %d",pid,num_pag,obtener_frame(num_pag,pid));
            paquete = crear_paquete(MARCO_LISTO);
            enviar_paquete(paquete,*socket_kernel);
            
            break;
        
        case LIBERAR_ESTRUCTURAS:

            memcpy(&pid, buffer->stream + desplazamiento , sizeof(int));
            finalizar_proceso(pid);
            paquete = crear_paquete(LIBERADAS);
            enviar_paquete(paquete,*socket_kernel);

            break;
        default:
        
            log_error(memoria, "El cliente KERNEL se desconectó.");
            break;
        }
        liberar_buffer(buffer);
    }
   
    

    
}
