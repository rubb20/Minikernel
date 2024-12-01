/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"
//Añadido para A2: comparaciones de nombres de mutex
#include <string.h>
#include <stdlib.h>

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;						/* ident. del proceso */
        int estado;					/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	/* copia de regs. de UCP */
        void * pila;				/* dir. inicial de la pila */
		BCPptr siguiente;			/* puntero a otro BCP */
		void *info_mem;				/* descriptor del mapa de memoria */
		//Añadido por la práctica:
		unsigned int segs_dormir;	/* segundos que debe dormir el proceso*/
		//A2: se añade que cada proceso tenga acceso al descriptor de mutex.
		int descriptores_mutex[NUM_MUT_PROC];
		//A3: ticks de Round-Robin
		int ticks;
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};

/*
 * Variable global que representa la cola de procesos dormidos
 */
lista_BCPs lista_dormidos= {NULL, NULL};

#define NO_RECURSIVO 0
#define RECURSIVO 1

/*
 * Estados posibles de un mutex
 */
#define MUT_NO_CREADO 0
#define MUT_DESBLOQUEADO 1
#define MUT_BLOQUEADO 2

typedef struct Mutex_t
{
		unsigned int id;			//Id del mutex (indice de la tabla_mutex en el que se encuentra).
		char *nombre;				//Nombre del mutex.
		int estado;					//Estado del mutex.
		int tipo;					//NO_RECURSIVO = 0, RECURSIVO = 1.
		BCP *proceso_bloqueador;	//Se guarda el proceso que ha bloqueado el mutex (proceso "propietario").
		int veces_bloq;	//Si es no_recursivo solo tomará 0 o 1, si es recursivo puede tomar desde 0 hasta MAX_INT
		lista_BCPs procesos_bloqueados; //Se guardan los procesos bloqueados por intentar obtener el mutex siendo no propietarios
} Mutex;

/*
 * Variable global que representa la tabla de mutex
 */

Mutex tabla_mutex[NUM_MUT];
/*
 * Variable global que representa la lista de procesos bloqueados por la función crear_mutex() debido a número máximo de mutex ya creados
 */
lista_BCPs lista_bloqueados = {NULL, NULL};

/*
 *
 * Definicion del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;


/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int sis_obtener_id();	//A0: servicio para obtener el id.
int sis_dormir();		//A1: servicio para dormir (bloquear) el proceso.
//A2: servicios para gestion del mutex
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock();
int sis_unlock();
int sis_cerrar_mutex();
/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir}, 
					{sis_obtener_id},
					{sis_dormir},
					{sis_crear_mutex},
					{sis_abrir_mutex},
					{sis_lock},
					{sis_unlock},
					{sis_cerrar_mutex} };

#endif /* _KERNEL_H */

