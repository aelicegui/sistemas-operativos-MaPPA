#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

//NUESTRAS
#include "sockets/sockets.h"
#include "pcb.h"
#include "leer_configs.h"

extern int grado_multi;
// -----------------------------------
//Funciones generales para la consola
// -----------------------------------
void iniciarProceso(char *mensaje);
void finalizarProcesoPID(char *mensaje);
void detenerPlanificacion();
void iniciarPlanificacion();
void modificarGradoMultiprogramacion(char *mensaje); 
void listarProcesosPorEstado();
t_mensajes_consola mensaje_a_consola(char *mensaje_consola);
void sacar_pcb_de_lista(t_pcb* pcb);
void sacar_de_lista(t_list * lista, int pid);

// -----------------------------------
// Consola
// -----------------------------------
void consola();
//void iniciar_consola();


#endif