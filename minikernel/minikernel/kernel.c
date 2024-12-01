/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */

/*
 * Funciones relacionadas con la tabla de mutex:
 *  iniciar_tabla_mutex buscar_mutex_libre
 */
static void iniciar_tabla_mutex()
{
	int i;

	for(i = 0; i < NUM_MUT; i++)
		tabla_mutex[i].estado = MUT_NO_CREADO;
}

static int buscar_mutex_libre()
{
	int i;

	for(i = 0; i<NUM_MUT; i++)
		if (tabla_mutex[i].estado == MUT_NO_CREADO)
			return (i);
	return (-1);
}
/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	for (int i = 0; i < NUM_MUT_PROC; i++)
	{
		if (p_proc_actual->descriptores_mutex[i] != -1) //Si el proceso tiene un descriptor de mutex asociado entonces
		{
			escribir_registro(1, (long)i); //Escribir en el registro 1 el descriptor.
			printk("Se va a liberar el descriptor %d\n", i);
			sis_cerrar_mutex();
		}
	}
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();
	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no deber�a llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char car;

	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

        return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){

	printk("-> TRATANDO INT. DE RELOJ\n");
	BCP *lista = lista_dormidos.primero;

	while (lista != NULL)
	{
		lista->segs_dormir--;
		if (lista->segs_dormir <= 0)
		{
			lista->estado = LISTO;
			lista->ticks = TICKS_POR_RODAJA;
			eliminar_elem(&lista_dormidos, lista);
			insertar_ultimo(&lista_listos, lista);
		}
		lista = lista->siguiente;
	}
	if (p_proc_actual->estado == LISTO)
	{
		p_proc_actual->ticks--;
		if (p_proc_actual->ticks <= 0)
		{
			activar_int_SW();
		}
	}
        return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){

	BCP *p_proc;
	p_proc = p_proc_actual;
	printk("-> TRATANDO INT. SW\n");
	if (lista_listos.primero == lista_listos.ultimo)
	{
		p_proc_actual->ticks = TICKS_POR_RODAJA;
	}
	else
	{
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&lista_listos, p_proc_actual);

		p_proc_actual = planificador();
		p_proc_actual->ticks = TICKS_POR_RODAJA;
		cambio_contexto(&(p_proc->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		p_proc->segs_dormir=0; //A1: se inicializa el atributo
		/* lo inserta al final de cola de listos */
		//A2: se inicializan los descriptores a -1, un número que identifica que NO hay ningun mutex asociado.
		for(int i = 0; i < NUM_MUT_PROC; i++)
			p_proc->descriptores_mutex[i] = -1;
		p_proc->ticks = TICKS_POR_RODAJA;
	
		insertar_ultimo(&lista_listos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no deber�a llegar aqui */
}

/* A0
 * Tratamiento de llamada al sistema obtener_id. Devuelve el miembro id del proceso actual.
 * No hace llamadas a funciones
 */
int sis_obtener_id(){
	return p_proc_actual->id;
}

/* A1
 * Tratamiento de la llamada al sistema dormir.
 * Hace que el SO sea multiprogramado.
 */
int sis_dormir(){
	BCP				*p_proc_anterior;
	unsigned int	segundos;

	segundos = leer_registro(1); //Leer del registro la información sobre los segundos que debe dormir el proceso
	p_proc_actual->segs_dormir = segundos * TICK;
	p_proc_actual->estado = BLOQUEADO;
	p_proc_anterior = p_proc_actual;
	eliminar_elem(&lista_listos, p_proc_actual);
	insertar_ultimo(&lista_dormidos, p_proc_actual);

	p_proc_actual = planificador(); //Procesador multiprogramado, se pasa al siguiente proceso. Es necesario cambiar el contexto.
	if (p_proc_actual != p_proc_anterior) //Pueden ser el mismo proceso en caso de que no haya mas procesos y se acabe de despertar, si esto pasa no hay que cambiar el contexto.
		cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
	return (0);
}
/* A2
 * Tratamiento para crear un mutex.
 *
*/
int sis_crear_mutex()
{
	int id;
	char *nombre=(char *)leer_registro(1);
	int	tipo = (int)leer_registro(2);
	int descriptor;
	BCP *p_proc_anterior;

	descriptor = -1;

	if ( (tipo != NO_RECURSIVO && tipo != RECURSIVO) || (nombre == NULL || strlen(nombre) > MAX_NOM_MUT)) //Si el tipo no es correcto
	{
		printk("Tipo de mutex no correcto o nombre muy largo\n");
		return (-1);
	}
	for(int i = 0; i < NUM_MUT; i++)
	{
		if (tabla_mutex[i].estado != MUT_NO_CREADO && strcmp(nombre, tabla_mutex[i].nombre) == 0) //Si se encuentra un mutex creado con nombre igual
		{
			printk("Ya hay un mutex con ese nombre\n");
			return (-1);
		}
	}
	for (int i = 0; i < NUM_MUT_PROC && descriptor == -1; i++) //Se realiza la busqueda de descriptor libre
	{
		if (p_proc_actual->descriptores_mutex[i] == -1)
			descriptor = i;
	}
	if (descriptor == -1) //Si no se encuentra descriptor libre
	{
		printk("No hay descriptor libre\n");
		return (-1);
	}
	id = buscar_mutex_libre();
	if (id == -1)
	{
		printk("Se está bloqueando el proceso a causa de: número maximo de mutex.\n");
		p_proc_anterior = p_proc_actual;
		p_proc_actual->estado = BLOQUEADO;
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&lista_bloqueados, p_proc_actual);
		p_proc_actual = planificador();
		cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	id = buscar_mutex_libre();
	if (id != -1)
	{
		tabla_mutex[id].id = id;
		tabla_mutex[id].nombre = malloc(MAX_NOM_MUT + 1);
		strcpy(tabla_mutex[id].nombre,nombre);
		tabla_mutex[id].tipo = tipo;
		tabla_mutex[id].estado = MUT_DESBLOQUEADO;
		tabla_mutex[id].veces_bloq = 0;
		tabla_mutex[id].proceso_bloqueador = NULL;
		tabla_mutex[id].procesos_bloqueados.primero = NULL;
		tabla_mutex[id].procesos_bloqueados.ultimo = NULL;
		p_proc_actual->descriptores_mutex[descriptor] = id;
	}
	return (descriptor); //Se devuelve el descriptor.
}

int sis_abrir_mutex()
{
	char *nombre = (char *)leer_registro(1);
	int	id;
	int descriptor;

	id = -1;
	descriptor = -1;

	for(int i = 0; i < NUM_MUT && id == -1; i++)
	{
		if (tabla_mutex[i].estado != MUT_NO_CREADO && strcmp(nombre, tabla_mutex[i].nombre) == 0)
			id = i;
	}
	if (id == -1) //No se ha encontrado el mutex con ese nombre
		return (-1);
	for (int i = 0; i < NUM_MUT_PROC && descriptor == -1; i++)
	{
		if (p_proc_actual->descriptores_mutex[i] == -1)
			descriptor = i;
	}
	if (descriptor == -1) //No hay descriptor libre
		return (-1);
	
	p_proc_actual->descriptores_mutex[descriptor] = id;
	return (descriptor);
}
int sis_lock()
{
	unsigned int descriptor = (unsigned int) leer_registro(1);
	int mutexId;
	Mutex *mutex;
	
	if (descriptor >= 4 || p_proc_actual->descriptores_mutex[descriptor] == -1) //Descriptor incorrecto.
		return (-1);
	mutexId = p_proc_actual->descriptores_mutex[descriptor];
	mutex = &tabla_mutex[mutexId];

	printk("Pruebo a bloquear\n");
	if (mutex->estado == MUT_BLOQUEADO && mutex->proceso_bloqueador != p_proc_actual) //Si está bloqueado y el proceso bloqueador no es el actual: bloquear el proceso actual.
	{
		//Se bloquea el proceso.
		BCP *aux = p_proc_actual;
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&mutex->procesos_bloqueados, p_proc_actual);
		p_proc_actual = planificador();
		cambio_contexto(&(aux->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	if (mutex->estado == MUT_DESBLOQUEADO && (mutex->proceso_bloqueador == NULL || mutex->proceso_bloqueador == p_proc_actual))
	{
		printk("Se bloquea\n");
		mutex->estado = MUT_BLOQUEADO;
		mutex->proceso_bloqueador = p_proc_actual;
		mutex->veces_bloq = 1;
	}
	else if (mutex->estado == MUT_BLOQUEADO && mutex->proceso_bloqueador == p_proc_actual) 
	{
		if (mutex->tipo == RECURSIVO)	//Se bloquea otra vez.
			mutex->veces_bloq += 1;
		else							//Error, intento de bloqueo a un mutex no recursivo ya bloqueado.
			return (-1);
	}
	return (0);
}
int sis_unlock()
{
	unsigned int descriptor = (unsigned int) leer_registro(1);
	int mutexId;
	Mutex *mutex;
	
	if (descriptor >= 4 || p_proc_actual->descriptores_mutex[descriptor] == -1) //Descriptor incorrecto.
		return (-1);
	mutexId = p_proc_actual->descriptores_mutex[descriptor];
	mutex = &tabla_mutex[mutexId];

	if (mutex->estado == MUT_BLOQUEADO && mutex->proceso_bloqueador != p_proc_actual)
	{
		return (-1); //Si un proceso no propietario del mutex intenta desbloquearlo no se le permite.
	}
	else if (mutex->estado == MUT_BLOQUEADO && mutex->proceso_bloqueador == p_proc_actual)
	{
		mutex->veces_bloq--;
		printk("Veces bloqueado -1 ahora su valor es: %d\n", mutex->veces_bloq);
		if (mutex->veces_bloq == 0)
			mutex->estado = MUT_DESBLOQUEADO;
		if (mutex->estado == MUT_DESBLOQUEADO && mutex->procesos_bloqueados.primero != NULL)
		{
			printk("Desbloqueando...\n");
			BCP *aux = mutex->procesos_bloqueados.primero;
			eliminar_elem(&mutex->procesos_bloqueados, aux);
			insertar_ultimo(&lista_listos, aux);
			mutex->proceso_bloqueador = aux;
		}
	}
	return (0);
}
int sis_cerrar_mutex()
{
	unsigned int descriptor = (unsigned int) leer_registro(1);
	int mutexId;
	Mutex *mutex;
	int	liberar;
	BCP *bloqueado_restaurar;
	
	if (descriptor >= 4 || p_proc_actual->descriptores_mutex[descriptor] == -1) //Descriptor incorrecto.
		return (-1);
	mutexId = p_proc_actual->descriptores_mutex[descriptor];
	mutex = &tabla_mutex[mutexId];

	while (mutex->estado != MUT_DESBLOQUEADO && mutex->proceso_bloqueador == p_proc_actual) //SI el proceso que cierra el mutex lo tiene bloqueado, desbloquearlo las veces necesarias.
	{
		sis_unlock();
	}
	p_proc_actual->descriptores_mutex[descriptor] = -1;
	printk("Un mutex ha sido cerrado\n");
	liberar = 1;
	for (int i = 0; i < MAX_PROC && liberar == 1; i++) 
	{
    	if (tabla_procs[i].estado != NO_USADA)
		{
        	for (int j = 0; j < NUM_MUT_PROC && liberar == 1; j++)
			{
            	if (tabla_procs[i].descriptores_mutex[j] == mutexId)
				{
                	printk("No puede ser liberado por el proceso %d\n", i);
            	 	liberar = 0; 
            	}
        	}
    	}
	}
	if (liberar == 1)
	{
		printk("Se está liberando un mutex\n");
		tabla_mutex[mutexId].estado = MUT_NO_CREADO;
		if (lista_bloqueados.primero != NULL)
		{
			bloqueado_restaurar = lista_bloqueados.primero;

			eliminar_elem(&lista_bloqueados, bloqueado_restaurar);
			insertar_ultimo(&lista_listos, bloqueado_restaurar);

			bloqueado_restaurar->estado = LISTO;
		}
	}
	return (0);
}
/*
 *
 * Rutina de inicializacion invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */
	
	iniciar_tabla_mutex(); /* Añadido: inicia Mutex de tabla de mutex*/

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
