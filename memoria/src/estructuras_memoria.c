#include "../include/estructuras_memoria.h"



void *memoria_usuario;
t_list *tablas_procesos_globales;
t_list *tablas_por_proceso;
sem_t espacio_usuario;

void iniciar_estructuras()
{

    memoria_usuario = calloc(1, tam_memoria());// inicializa todos sus bytes a cero.
    sem_init(&espacio_usuario, 0, 1);
    sem_init(&terminaron_conexiones, 0, 0);
    log_info(memoria, "Memoria para el espacio de usuario creada!");
    
    tablas_por_proceso = list_create();//t_paginas
    //tablas_procesos_globales = list_create();//t_tabla_proceso
    procesos_de_memoria = list_create();
    lista_instrucciones_globales = list_create();

    vector_marcos = calloc(tam_memoria() / tam_pagina(), sizeof(int));

    for(int i=0; i< tam_memoria() / tam_pagina(); i++) //creo los marcos/paginas 
    {
       //t_pagina* pagina = NULL;
       //list_add(tablas_por_proceso,pagina);
       vector_marcos[i] =  0;
    }
    log_info(memoria, "Tabla de planificacion de %d marcos creada",(tam_memoria() / tam_pagina()));

}

int obtener_cant_pags(int tam_proceso)
{
     return ceil(tam_proceso / tam_pagina());
}

int asignar_frame_libre()
{
     for(int i=0; i<tam_memoria() / tam_pagina(); i++)
     {
          if(vector_marcos[i] == 0)
          {
               vector_marcos[i] = 1;
               return i;
          }
     }
    return -1;
}

void inicializar_tabla_paginas(t_proceso_memoria *proceso)
{
     
     int cant_pags = obtener_cant_pags(proceso->tamanio_proceso_bytes);
     
     int* bloques_asignados = solicitar_SWAP(proceso);     
     
     t_tabla_por_proceso *tabla_proceso = malloc(sizeof(t_tabla_por_proceso)); 
     tabla_proceso->pid = proceso->pcb->contexto_ejecucion->pid;
     tabla_proceso->tabla_paginas = list_create();

     int posicion_en_swap;


     for(int i = 0; i < cant_pags; i++)
     {
          t_pagina *pagina = (t_pagina*)malloc(sizeof(t_pagina));
          pagina -> marco = -1;
          pagina -> presencia = 0;
          pagina -> modificado = 0;
          posicion_en_swap = bloques_asignados[i] * tam_pagina();
          pagina -> pos_en_swap = posicion_en_swap;
          pagina -> tiempo_carga = 0; 
          pagina -> ultima_referencia = 0;
          pagina -> PID = proceso->pcb->contexto_ejecucion->pid;
          list_add(tabla_proceso->tabla_paginas, pagina);
     }
    
     list_add(tablas_por_proceso, tabla_proceso);

     log_info(logger_memoria,"PID: %d - Tamaño: %d",proceso->pcb->contexto_ejecucion->pid, cant_pags);
     
     free(bloques_asignados);
}

void finalizar_tabla_paginas(t_proceso_memoria *proceso)
{

     liberar_SWAP(proceso); //aca se hacer el free de bloques
   
     int PID = proceso->pcb->contexto_ejecucion->pid;
     
     t_tabla_por_proceso* tabla_proceso = obtener_tabla_proceso(PID);

     if(tabla_proceso != NULL)
     {    
          liberar_frames_asignados(PID);

          for(int i=0; i < (list_size(tabla_proceso->tabla_paginas)); i++)
          {    
               free(list_remove(tabla_proceso->tabla_paginas,i));
               
          }
          free(list_remove(tablas_por_proceso, indice_en_tabla(tabla_proceso)));

          return;
     }

     log_error(memoria,"No se ha encontrado la tabla de paginas asociada al proceso de PID: %d", PID);
     
}


void liberar_frames_asignados(int pid)
{
     t_list *tabla_paginas = obtener_tabla_pagina(pid);

     for(int i=0; i< (list_size(tabla_paginas)); i++)
     {
          t_pagina *pagina = list_get(tabla_paginas,i);

          if(pagina -> presencia)
          {
               int marco = pagina -> marco;
               vector_marcos[marco] = 0;
              
               pagina -> presencia = 0;
          }
     }
}

void liberar_instrucciones_proceso(t_instrucciones_proceso *instrucciones_proceso)
{
     for(int i=0; i<(list_size(instrucciones_proceso -> lista_instrucciones)); i++)
     {
          t_instruccion *instruccion = list_get(instrucciones_proceso -> lista_instrucciones, i);
          liberar_instruccion(instruccion);
     }

     bool resultado = list_remove_element(lista_instrucciones_globales,instrucciones_proceso);

     if(!resultado)
     {
          log_error(memoria,"No se encontro la estructura de instrucciones del proceso a liberar");
     }

     free(instrucciones_proceso);
}

//ALGORITMOS
int aplicar_algoritmo_FIFO()
{
     
     t_list *paginas_presentes = obtener_paginas_en_memoria();
     t_pagina* pagina = ((t_pagina*)list_get(paginas_presentes,0));
     time_t tiempo_primer_elemento = pagina -> tiempo_carga;
     int victima = pagina->marco;

     for(int i=1; i< (list_size(paginas_presentes)); i++)
     {
       time_t tiempo_otro_elemento = ((t_pagina*)list_get(paginas_presentes,i)) -> tiempo_carga;
       if(tiempo_otro_elemento < tiempo_primer_elemento)
       {
          tiempo_primer_elemento = tiempo_otro_elemento;
          victima = ((t_pagina*)list_get(paginas_presentes,i)) -> marco;
       }
     }
     return victima;
}

int aplicar_algoritmo_LRU()
{
     t_list *paginas_presentes = obtener_paginas_en_memoria();//OBTENGO LAS PAGINAS EN MEMORIA
     t_pagina* pagina = ((t_pagina*)list_get(paginas_presentes,0)); //TOMO UNA COMO PARA COMPARAR
     time_t acceso_primer_elemento = pagina -> ultima_referencia;//TOMO EL ULTIMO ACCESO
     int victima = pagina->marco;
     
     for(int i=1; i< (list_size(paginas_presentes)); i++)
     {
          time_t acceso_otro_elemento = ((t_pagina*)list_get(paginas_presentes,i)) -> ultima_referencia; //TOMO UNA PAGINA Y SI ULTIMA REFERENCIA
          if(acceso_otro_elemento < acceso_primer_elemento)//un valor de time_t mayor representa un momento más antiguo en el tiempo.
          {
               acceso_primer_elemento = acceso_otro_elemento;
               victima = ((t_pagina*)list_get(paginas_presentes,i)) -> marco;
          }
     }

     return victima;
}



//ATAJOS
t_list *obtener_paginas_en_memoria()
{
     t_list *paginas_en_memoria = list_create();

     for(int i=0; i< (list_size(tablas_por_proceso));i++)
     {
          t_tabla_por_proceso *tablas_paginas_pid = list_get(tablas_por_proceso,i);

          for (int j=0 ;j< (list_size(tablas_paginas_pid -> tabla_paginas));  j++)
          {
               t_pagina *pagina = list_get((tablas_paginas_pid -> tabla_paginas), j);

               if(pagina -> presencia == 1)
               {
                    list_add(paginas_en_memoria, pagina);
               }
          }
          
          
     }

     return paginas_en_memoria;
}

t_pagina *obtener_pagina(int frame)
{
     t_list *pags_en_memoria =  obtener_paginas_en_memoria();

     for(int i=0; i < (list_size(pags_en_memoria)); i++)
     {
          t_pagina *pagina = list_get(pags_en_memoria,i);

          if(pagina -> marco == frame)
          {
               return pagina;
          }
     }

     return NULL;
     
}

t_list *obtener_tabla_pagina(int pid)
{
     int cant_elem_list = list_size(tablas_por_proceso);
     t_tabla_por_proceso *tabla_pagina_proceso;

     for(int i=0 ; i< cant_elem_list ; i++){
          tabla_pagina_proceso = list_get(tablas_por_proceso,i);
          if(tabla_pagina_proceso->pid == pid){
               return tabla_pagina_proceso->tabla_paginas;
          }
     }
     return NULL;
}

int obtener_frame(int num_pag, int pid)
{
     t_list *tabla_paginas = obtener_tabla_pagina(pid);

     t_pagina *pagina = list_get(tabla_paginas,num_pag);

     return pagina -> marco;
}

int get_index(int pid, int frame)
{    
     t_list *tabla_paginas = obtener_tabla_pagina(pid);

    for(int i=0; i<(list_size(tabla_paginas)); i++)
    {   
        t_tabla_por_proceso *tablas_pagina_pid = list_get(tablas_por_proceso, i);
        for(int j=0 ; j < list_size(tablas_pagina_pid-> tabla_paginas);j++)
        { 
          t_pagina *pagina = list_get((tablas_pagina_pid-> tabla_paginas),j);
          if(pagina -> marco == frame)
          {
               return i;
          }
        }
       
    }

    return -1;
}

t_tabla_por_proceso *obtener_tabla_proceso(int pid_asociado)
{   
     for(int i=0; i<(list_size(tablas_por_proceso));i++)
     {
          t_tabla_por_proceso *tabla_proceso = list_get(tablas_por_proceso,i);
          if(tabla_proceso -> pid == pid_asociado)
          {
               return tabla_proceso;
          }
     }
     return NULL;
}
int indice_en_tabla(t_tabla_por_proceso *tabla_proceso){

     for(int i=0; i < (list_size(tablas_por_proceso)); i++){
          t_tabla_por_proceso *tabla_aux = list_get(tablas_por_proceso,i);
          if( tabla_aux -> pid == (tabla_proceso -> pid)){
               return i;
          }
     }

     return -1;
}






