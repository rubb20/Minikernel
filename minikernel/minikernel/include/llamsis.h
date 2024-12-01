/*
 *  minikernel/kernel/include/llamsis.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 *
 * 	SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

/* Numero de llamadas disponibles */
#define NSERVICIOS 10

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
#define OBTENER_ID 3 //A0: se define para realizar la llamada al sistema (en el array de llamadas la llamada estará en el 3).
#define DORMIR 4 //A1: se define la llamada al sistema (en el array estará en el 4).
#define CREAR_MUTEX 5
#define ABRIR_MUTEX 6
#define LOCK_MUTEX 7
#define UNLOCK_MUTEX 8
#define CERRAR_MUTEX 9

#endif /* _LLAMSIS_H */

