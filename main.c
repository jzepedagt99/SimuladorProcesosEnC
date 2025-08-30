#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


typedef struct Proceso {
    int pid;
    int ppid;
    char nombre[50];
    int registros;
    int tamano;
    int hilos;
    int quantum;
    int iteraciones;

    int quantumRestante;
    int iteracionesRestantes;
    bool terminado;

    struct Proceso *sig;
} Proceso;

typedef struct CPU {
    int id;
    Proceso *actual;
} CPU;

// ==========================
// Variables globales
// ==========================
int procesadores = 0;
int hilos = 0;
CPU *cpus = NULL;

Proceso *colaListos = NULL;
Proceso *colaTerminados = NULL;

// Encolar al final
void encolar(Proceso **cola, Proceso *p) {
    p->sig = NULL;
    if (*cola == NULL) {
        *cola = p;
    } else {
        Proceso *tmp = *cola;
        while (tmp->sig != NULL) tmp = tmp->sig;
        tmp->sig = p;
    }
}

// Desencolar al inicio
Proceso* desencolar(Proceso **cola) {
    if (*cola == NULL) return NULL;
    Proceso *p = *cola;
    *cola = (*cola)->sig;
    p->sig = NULL;
    return p;
}

// Ver si alguna CPU está ocupada
bool hayProcesosEnCPU(int numCPUs) {
    for (int i = 0; i < numCPUs; i++) {
        if (cpus[i].actual != NULL) return true;
    }
    return false;
}

// Crear un proceso
Proceso* crearProceso(int pid, int ppid, const char *nombre,
                      int registros, int tamano, int hilos,
                      int quantum, int iteraciones) {
    Proceso *p = (Proceso*)malloc(sizeof(Proceso));
    p->pid = pid;
    p->ppid = ppid;
    strncpy(p->nombre, nombre, sizeof(p->nombre)-1);
    p->registros = registros;
    p->tamano = tamano;
    p->hilos = hilos;
    p->quantum = quantum;
    p->iteraciones = iteraciones;
    p->quantumRestante = quantum;
    p->iteracionesRestantes = iteraciones;
    p->terminado = false;
    p->sig = NULL;
    return p;
}

// ==========================
// Lectura de archivo
// ==========================
void leerArchivo(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Error al abrir archivo");
        exit(1);
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), f)) {
        if (strncmp(linea, "Procesadores", 12) == 0) {
            sscanf(linea, "Procesadores [%d]", &procesadores);
        } else if (strncmp(linea, "Hilos", 5) == 0) {
            sscanf(linea, "Hilos [%d]", &hilos);
            break;
        }
    }

    printf("Listado de procesos listos \n\n");
    printf("[PID] | [PPID] | [Nombre] | [Reg] | [Tam] | [Hilos] | [Quantum] | [Iter]\n");

    while (fgets(linea, sizeof(linea), f)) {
        if (strlen(linea) <= 1) continue;
        printf("%s", linea);

        int pid, ppid, registros, tamano, hilosP, quantum, iteraciones;
        char nombre[50];
        sscanf(linea, "%d | %d | %49s | %d | %d | %d | %d | %d",
               &pid, &ppid, nombre, &registros, &tamano, &hilosP, &quantum, &iteraciones);

        Proceso *p = crearProceso(pid, ppid, nombre, registros, tamano, hilosP, quantum, iteraciones);
        encolar(&colaListos, p);
    }

    fclose(f);
}

// ==========================
// Inicializar CPUs
// ==========================
void inicializarCPUs() {
    int total = procesadores * hilos;
    cpus = (CPU*)malloc(sizeof(CPU) * total);
    for (int i = 0; i < total; i++) {
        cpus[i].id = i;
        cpus[i].actual = NULL;
    }
}

// ==========================
// Simulación
// ==========================
void simularSchedule() {
    int pulso = 0;
    int numCPUs = procesadores * hilos;
    printf("\n\n\n");
    while (colaListos != NULL || hayProcesosEnCPU(numCPUs)) {
        printf("Pulso %d\n", pulso);

        // Liberar CPUs que terminaron quantum
        for (int i = 0; i < numCPUs; i++) {
            if (cpus[i].actual != NULL) {
                Proceso *p = cpus[i].actual;
                p->quantumRestante--;
                if (p->quantumRestante == 0) {
                    p->iteracionesRestantes--;
                    if (p->iteracionesRestantes > 0) {
                        p->quantumRestante = p->quantum;
                        encolar(&colaListos, p);
                    } else {
                        p->terminado = true;
                        encolar(&colaTerminados, p);
                    }
                    cpus[i].actual = NULL;
                }
            }
        }

        // Asignar procesos a CPUs libres
        for (int i = 0; i < numCPUs; i++) {
            if (cpus[i].actual == NULL && colaListos != NULL) {
                cpus[i].actual = desencolar(&colaListos);
            }
        }

        // Mostrar estado
        for (int i = 0; i < numCPUs; i++) {
            if (cpus[i].actual != NULL) {
                Proceso *p = cpus[i].actual;
                printf("  CPU%02d: PID=%d (%s, Quantum=%d, Iteracion=%d)\n",
                       cpus[i].id, p->pid, p->nombre,
                       p->quantumRestante, p->iteracionesRestantes);
            } else {
                printf("  CPU%02d: (libre)\n", cpus[i].id);
            }
        }

        pulso++;
        printf("\n");
    }

    // Conteo final
    int terminados = 0;
    Proceso *tmp = colaTerminados;
    while (tmp != NULL) {
        terminados++;
        tmp = tmp->sig;
    }
    printf("Simulación terminada.\n");
    printf("Procesos terminados: %d\n", terminados);
}

int main()
{
    leerArchivo("configuracion.txt");
    inicializarCPUs();
    simularSchedule();
    return 0;
}
