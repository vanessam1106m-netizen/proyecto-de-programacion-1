#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// incluimos el archivo de combate de ellas (que ya tiene los enum y structs)
#include "combate_respaldo.c"

// incluimos nuestros modulos de logica
#include "archivos.c"
#include "backtracking.c"

// variables globales del programa
Pokedex pokedex[150];
int total_pokedex = 0;

Entrenador entrenadores[100];
EjemplarPokemon equipos_entrenadores[100][6];
int cant_pokemon_entrenador[100];
int total_entrenadores = 0;

int secuencia_iu = 1;

// funcion sencilla para limpiar lo que sobra en scanf
void limpiar_teclado() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// muestra la lista de las 150 especies de la pokedex
void ver_pokedex() {
    int i;
    printf("\n============================= POKEDEX =============================\n");
    printf("%-5s %-15s %-12s %-12s %-5s %-5s %-5s %-5s\n",
           "Num", "Nombre", "Tipo 1", "Tipo 2", "HP", "ATK", "DEF", "VEL");
    printf("-------------------------------------------------------------------\n");
    for (i = 0; i < total_pokedex; i++) {
        printf("#%03d %-15s %-12s %-12s %-5d %-5d %-5d %-5d\n",
               pokedex[i].N_Pokedex,
               pokedex[i].Nombre_de_pokedex,
               pokedex[i].T_Primario,
               strlen(pokedex[i].T_Secundario) > 0 ? pokedex[i].T_Secundario : "-",
               pokedex[i].Hp_base,
               pokedex[i].Ataque_base,
               pokedex[i].Defensa_base,
               pokedex[i].Velocidad_base);
    }
}

// lista los entrenadores que estan anotados
void listar_entrenadores() {
    int i;
    if (total_entrenadores == 0) {
        printf("\nNo hay entrenadores registrados todavia.\n");
        return;
    }

    printf("\n==================== LISTA DE ENTRENADORES ====================\n");
    printf("%-6s %-20s %-8s %-5s %-5s %-5s %-8s\n", "ID", "Nombre", "Pokemon", "V", "E", "D", "Puntos");
    printf("----------------------------------------------------------------\n");
    for (i = 0; i < total_entrenadores; i++) {
        printf("%-6d %-20s %-8d %-5d %-5d %-5d %-8d\n",
               entrenadores[i].Identificador,
               entrenadores[i].Nombre,
               cant_pokemon_entrenador[i],
               entrenadores[i].Cantidad_de_victorias,
               entrenadores[i].Cantidad_de_empates,
               entrenadores[i].Cantidad_de_derrotas,
               entrenadores[i].Puntuacion);
    }
}

// muestra los datos de un entrenador y los pokemon de su equipo
void ver_detalle_entrenador(int pos) {
    int i;
    Entrenador *e = &entrenadores[pos];
    int cantidad = cant_pokemon_entrenador[pos];

    printf("\n--------------------------------------------------------------\n");
    printf("Entrenador: %s (ID: %d)\n", e->Nombre, e->Identificador);
    printf("Victorias: %d | Empates: %d | Derrotas: %d | Puntos: %d\n",
           e->Cantidad_de_victorias, e->Cantidad_de_empates, e->Cantidad_de_derrotas, e->Puntuacion);
    printf("Equipo Pokemon (%d/6):\n", cantidad);
    
    for (i = 0; i < cantidad; i++) {
        EjemplarPokemon *p = &equipos_entrenadores[pos][i];
        printf("  %d) [IU:%d] %-12s Nivel %-2d | HP: %d | ATK: %d DEF: %d VEL: %d | Tipos: %s (%d) / %s (%d)\n",
               i + 1, p->IU, p->Nombre, p->Nivel,
               p->Hp, p->Ataque, p->Defensa, p->Velocidad,
               nombres_tipos[p->Valor_T_Primario], p->Valor_T_Primario,
               nombres_tipos[p->Valor_T_Secundario], p->Valor_T_Secundario);
    }
    printf("--------------------------------------------------------------\n");
}

// opcion para dar de alta a un entrenador
void opcion_registrar_entrenador() {
    int id, i;
    char nombre[50];

    printf("\n--- REGISTRAR ENTRENADOR ---\n");
    printf("Ingrese numero de identificador unico: ");
    if (scanf("%d", &id) != 1) {
        limpiar_teclado();
        printf("Numero invalido.\n");
        return;
    }
    limpiar_teclado();

    // revisar que el id no este repetido
    for (i = 0; i < total_entrenadores; i++) {
        if (entrenadores[i].Identificador == id) {
            printf("Error: Ya existe un participante con el ID %d.\n", id);
            return;
        }
    }

    printf("Ingrese nombre del participante: ");
    if (scanf("%49[^\n]", nombre) != 1) {
        limpiar_teclado();
        printf("Nombre invalido.\n");
        return;
    }
    limpiar_teclado();

    Entrenador *nuevo = &entrenadores[total_entrenadores];
    nuevo->Identificador = id;
    strcpy(nuevo->Nombre, nombre);
    nuevo->Cantidad_de_victorias = 0;
    nuevo->Cantidad_de_empates = 0;
    nuevo->Cantidad_de_derrotas = 0;
    nuevo->Puntuacion = 0;
    cant_pokemon_entrenador[total_entrenadores] = 0;

    total_entrenadores++;
    printf("Entrenador '%s' guardado correctamente.\n", nombre);

    guardar_entrenadores("entrenadores.txt", entrenadores, equipos_entrenadores, cant_pokemon_entrenador, total_entrenadores);
}

// opcion para armarle el equipo a un entrenador
void opcion_crear_equipo() {
    if (total_entrenadores == 0) {
        printf("\nPrimero registra algun entrenador.\n");
        return;
    }

    int id, i, pos = -1;
    listar_entrenadores();
    printf("\nEscribe el ID del entrenador: ");
    if (scanf("%d", &id) != 1) {
        limpiar_teclado();
        return;
    }
    limpiar_teclado();

    for (i = 0; i < total_entrenadores; i++) {
        if (entrenadores[i].Identificador == id) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        printf("No se encontro ese ID.\n");
        return;
    }

    int modo;
    printf("\nElige como armar el equipo:\n");
    printf("1. Manual (uno a uno)\n");
    printf("2. Automatico con Backtracking\n");
    printf("Opcion: ");
    if (scanf("%d", &modo) != 1) {
        limpiar_teclado();
        return;
    }
    limpiar_teclado();

    if (modo == 1) {
        int cantidad;
        printf("Cuantos Pokemon quieres? (1 a 6): ");
        if (scanf("%d", &cantidad) != 1 || cantidad < 1 || cantidad > 6) {
            limpiar_teclado();
            printf("Numero invalido.\n");
            return;
        }
        limpiar_teclado();

        cant_pokemon_entrenador[pos] = 0;
        for (i = 0; i < cantidad; i++) {
            int num, nivel, k, encontrado = -1;
            printf("\nPokemon #%d:\n", i + 1);
            printf("  Numero de Pokedex (1 al 150): ");
            scanf("%d", &num);
            printf("  Nivel (1 al 100): ");
            scanf("%d", &nivel);
            limpiar_teclado();

            for (k = 0; k < total_pokedex; k++) {
                if (pokedex[k].N_Pokedex == num) {
                    encontrado = k;
                    break;
                }
            }

            if (encontrado != -1 && nivel >= 1 && nivel <= 100) {
                instanciar_ejemplar(pokedex[encontrado], nivel, &equipos_entrenadores[pos][cant_pokemon_entrenador[pos]], &secuencia_iu);
                cant_pokemon_entrenador[pos]++;
                printf("  -> %s agregado!\n", pokedex[encontrado].Nombre_de_pokedex);
            } else {
                printf("  Datos erroneos, repite esta entrada.\n");
                i--;
            }
        }
        printf("\nEquipo listo!\n");
        ver_detalle_entrenador(pos);
        guardar_entrenadores("entrenadores.txt", entrenadores, equipos_entrenadores, cant_pokemon_entrenador, total_entrenadores);

    } else if (modo == 2) {
        Restricciones r;
        printf("\n--- REGLAS PARA EL BACKTRACKING ---\n");
        printf("Cantidad de integrantes (ej 3 o 6): ");
        scanf("%d", &r.cantidad_pokemon);
        printf("Nivel total maximo (ej 80 o 100, 0 para no limitar): ");
        scanf("%d", &r.nivel_max_total);
        printf("Minimo de tipos distintos (ej 3, 0 para no exigir): ");
        scanf("%d", &r.tipos_minimos);
        printf("No repetir misma especie? (1 = Si, 0 = No): ");
        scanf("%d", &r.no_repetir_especie);
        r.hp_min_total = 0;
        r.ataque_min_total = 0;
        r.defensa_min_total = 0;
        r.velocidad_min_total = 0;
        limpiar_teclado();

        // tomamos un lote variado de candidatos
        EjemplarPokemon candidatos[60];
        int total_cand = 0;
        for (i = 0; i < 60 && i < total_pokedex; i++) {
            int nivel = 10 + (i % 15);
            instanciar_ejemplar(pokedex[i], nivel, &candidatos[total_cand], &secuencia_iu);
            total_cand++;
        }

        EjemplarPokemon solucion[6];
        printf("\nBuscando combinacion...\n");
        if (buscar_equipo_bt(candidatos, total_cand, 0, solucion, 0, r)) {
            printf("Combinacion encontrada con exito!\n");
            cant_pokemon_entrenador[pos] = r.cantidad_pokemon;
            for (i = 0; i < r.cantidad_pokemon; i++) {
                equipos_entrenadores[pos][i] = solucion[i];
            }
            ver_detalle_entrenador(pos);
            guardar_entrenadores("entrenadores.txt", entrenadores, equipos_entrenadores, cant_pokemon_entrenador, total_entrenadores);
        } else {
            printf("\nNo se pudo armar el equipo: no hay combinacion que cumpla todas esas condiciones.\n");
        }
    }
}

// opcion para ver el equipo de un entrenador
void opcion_consultar_equipo() {
    if (total_entrenadores == 0) {
        printf("\nNo hay entrenadores registrados.\n");
        return;
    }
    int id, i;
    listar_entrenadores();
    printf("\nEscribe el ID del entrenador que quieres revisar: ");
    if (scanf("%d", &id) == 1) {
        limpiar_teclado();
        for (i = 0; i < total_entrenadores; i++) {
            if (entrenadores[i].Identificador == id) {
                ver_detalle_entrenador(i);
                return;
            }
        }
        printf("No se encontro ese ID.\n");
    } else {
        limpiar_teclado();
    }
}

// opcion para enfrentar a los dos primeros pokemon de dos entrenadores usando el motor de ellas
void opcion_combate() {
    if (total_entrenadores < 2) {
        printf("\nNecesitas al menos 2 entrenadores para hacer un combate.\n");
        return;
    }

    int id1, id2, i;
    int pos1 = -1, pos2 = -1;

    listar_entrenadores();
    printf("\nID del primer entrenador: ");
    scanf("%d", &id1);
    printf("ID del segundo entrenador: ");
    scanf("%d", &id2);
    limpiar_teclado();

    for (i = 0; i < total_entrenadores; i++) {
        if (entrenadores[i].Identificador == id1) pos1 = i;
        if (entrenadores[i].Identificador == id2) pos2 = i;
    }

    if (pos1 == -1 || pos2 == -1 || pos1 == pos2) {
        printf("Error: Seleccionaste entrenadores invalidos o iguales.\n");
        return;
    }

    if (cant_pokemon_entrenador[pos1] == 0 || cant_pokemon_entrenador[pos2] == 0) {
        printf("Ambos entrenadores deben tener al menos un Pokemon en su equipo.\n");
        return;
    }

    EjemplarPokemon *p1 = &equipos_entrenadores[pos1][0];
    EjemplarPokemon *p2 = &equipos_entrenadores[pos2][0];

    printf("\n=== COMBATE 1v1 ===\n");
    printf("%s usa a %s (HP: %d)\n", entrenadores[pos1].Nombre, p1->Nombre, p1->Hp);
    printf("%s usa a %s (HP: %d)\n\n", entrenadores[pos2].Nombre, p2->Nombre, p2->Hp);

    int ganador = simular_combate_1v1(p1, p2);

    if (ganador == 1) {
        printf("\nGanador: %s! Su %s quedo con %d HP.\n", entrenadores[pos1].Nombre, p1->Nombre, p1->Hp);
        entrenadores[pos1].Cantidad_de_victorias++;
        entrenadores[pos1].Puntuacion += 3;
        entrenadores[pos2].Cantidad_de_derrotas++;
    } else if (ganador == 2) {
        printf("\nGanador: %s! Su %s quedo con %d HP.\n", entrenadores[pos2].Nombre, p2->Nombre, p2->Hp);
        entrenadores[pos2].Cantidad_de_victorias++;
        entrenadores[pos2].Puntuacion += 3;
        entrenadores[pos1].Cantidad_de_derrotas++;
    } else {
        printf("\nEl duelo quedo en empate!\n");
        entrenadores[pos1].Cantidad_de_empates++;
        entrenadores[pos2].Cantidad_de_empates++;
        entrenadores[pos1].Puntuacion += 1;
        entrenadores[pos2].Puntuacion += 1;
    }

    guardar_entrenadores("entrenadores.txt", entrenadores, equipos_entrenadores, cant_pokemon_entrenador, total_entrenadores);
}

// punto de entrada del torneo
int main_torneo() {
    printf("Cargando datos del torneo...\n");

    // 1. cargar pokedex
    if (!cargar_pokedex("pokedex.txt", pokedex, &total_pokedex)) {
        printf("Aviso: 'pokedex.txt' no se pudo leer.\n");
    } else {
        printf("Pokedex lista con %d especies.\n", total_pokedex);
    }

    // 2. cargar entrenadores guardados
    if (cargar_entrenadores("entrenadores.txt", entrenadores, equipos_entrenadores, cant_pokemon_entrenador, &total_entrenadores, &secuencia_iu)) {
        printf("Se cargaron %d entrenadores guardados.\n", total_entrenadores);
    }

    int op = 0;
    while (op != 7) {
        printf("\n========================================\n");
        printf("          GRAN TORNEO POKEMON           \n");
        printf("========================================\n");
        printf("1. Ver Pokedex\n");
        printf("2. Registrar entrenador\n");
        printf("3. Crear equipo (Manual o Backtracking)\n");
        printf("4. Listar entrenadores\n");
        printf("5. Ver equipo de un entrenador\n");
        printf("6. Pelea 1v1 entre dos entrenadores\n");
        printf("7. Salir\n");
        printf("========================================\n");
        printf("Elige una opcion: ");

        if (scanf("%d", &op) != 1) {
            limpiar_teclado();
            printf("Entrada no valida.\n");
            continue;
        }
        limpiar_teclado();

        switch (op) {
            case 1: ver_pokedex(); break;
            case 2: opcion_registrar_entrenador(); break;
            case 3: opcion_crear_equipo(); break;
            case 4: listar_entrenadores(); break;
            case 5: opcion_consultar_equipo(); break;
            case 6: opcion_combate(); break;
            case 7:
                printf("\nGuardando cambios y saliendo...\n");
                guardar_entrenadores("entrenadores.txt", entrenadores, equipos_entrenadores, cant_pokemon_entrenador, total_entrenadores);
                break;
            default:
                printf("Opcion incorrecta.\n");
                break;
        }
    }

    return 0;
}

// funcion principal
int main() {
    srand(time(NULL));
    return main_torneo();
}
