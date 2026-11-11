#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>

// Estructuras de ejemplo basadas en los requerimientos del proyecto
typedef struct {
    int id;
    int nivel;
    int hp_actual;
    int ataque;
    int defensa;
    int velocidad;
    char tipo1[20];
    char tipo2[20];
    int estado; // 1 = disponible/vivo, 0 = debilitado
} Ejemplar;

typedef struct {
    char id[20];
    char nombre[50];
    Ejemplar equipo[6]; // Asumiendo un equipo de hasta 6 Pokémon
    int cantidad_pokemones;
} Entrenador;

float obtener_multiplicador_tipos(char *tipo_atacante, char *tipo1_defensor, char *tipo2_defensor); // aun no se si implementar 
int calcular_dano(Ejemplar *atacante, Ejemplar *defensor);
int simular_combate_1v1(Ejemplar *p1, Ejemplar *p2);

int main (){
    srand(time(NULL));

    // Crear dos ejemplares de prueba
   Ejemplar p1 = {1, 10, 0, 15, 10, 120, "Fuego", "", 0}; // HP en 0 y estado 0
    Ejemplar p2 = {2, 10, 100, 12, 10, 80, "Agua", "", 1};

    printf("=== INICIO DEL DUELO ===\n");
    printf("P1 (HP: %d) vs P2 (HP: %d)\n\n", p1.hp_actual, p2.hp_actual);

    // Simular el combate
    int ganador = simular_combate_1v1(&p1, &p2);

    printf("=== FIN DEL DUELO ===\n");
    if (ganador == 1) {
        printf("¡El Ganador es el Ejemplar 1 (P1)! HP final: %d\n", p1.hp_actual);
    } else {
        printf("¡El Ganador es el Ejemplar 2 (P2)! HP final: %d\n", p2.hp_actual);
    }


    return 0;
}



float obtener_multiplicador_tipos(char *tipo_atacante, char *tipo1_defensor, char *tipo2_defensor){
    float multiplicador = 1.0; // Daño normal por defecto
    
    // TODO: Aquí cruzas el tipo del atacante con el tipo1_defensor 
    // (ej: si es muy efectivo *= 2.0, si es poco efectivo *= 0.5, etc.)
    
    // Si el defensor posee un segundo tipo, el reglamento exige evaluarlo también
    if (tipo2_defensor != NULL && tipo2_defensor[0] != '\0') {
        // TODO: Multiplicas o combinas con el factor correspondiente al segundo tipo
    }
    
    return multiplicador;
}


// Función principal para el cálculo del daño
int calcular_dano(Ejemplar *atacante, Ejemplar *defensor){
    int dano_base, dano_final;
    float multiplicador_tipos, dano_calculado;
    // 1. Fórmula base propuesta: (Nivel * Ataque) - Defensa
    dano_base = (atacante->nivel * atacante->ataque) - defensor->defensa;
    
    // 2. Umbral de seguridad mínimo: si la defensa es muy alta, aseguramos al menos 2 puntos de daño
    if(dano_base <= 0){
        dano_base = 2;
    }
    
    // 3. Ponderar el resultado multiplicándolo por el factor de los tipos (primario y secundario)
    multiplicador_tipos = obtener_multiplicador_tipos(atacante->tipo1, defensor->tipo1, defensor->tipo2);
    dano_calculado = (dano_base * multiplicador_tipos);
    dano_final = (int)round(dano_calculado);
    
    // Garantizar un daño mínimo de 1 si el tipo no anuló por completo el ataque (multiplicador > 0)
    if(dano_final < 1 && multiplicador_tipos > 0){
        dano_final = 2;
    }
    
    return dano_final;
}


// SIMULACIÓN DE COMBATE 1v1
// Retorna: 1 si gana p1, 2 si gana p2
int simular_combate_1v1(Ejemplar *p1, Ejemplar *p2){
    int duelo_activo = 1, dano1, dano2;
    int ganador = 0, turno;

    // Verificar si alguno ingresó ya sin HP
    if (p1->hp_actual <= 0) {
        duelo_activo = 0;
        ganador = 2;
    } else if (p2->hp_actual <= 0) {
        duelo_activo = 0;
        ganador = 1;
    }

    // Determinar la iniciativa (quién ataca primero en la primera ronda) evaluando velocidades
    if(duelo_activo){ // 1 = p1 y 0 = p2
        if(p1->velocidad > p2->velocidad){
            turno = 1;
        } else if(p2->velocidad > p1->velocidad){
            turno = 0;
        } else{ // Desempate aleatorio si tienen la misma velocidad   
            turno = rand() % 2;
        }
    }

    // Ciclo principal de turnos entre ambos ejemplares[cite: 6]
    while(duelo_activo){

        if(turno == 1){
    // --- TURNO DE P1 ---
        dano1 = calcular_dano(p1, p2);
        p2->hp_actual -= dano1;
        
        if(p2->hp_actual <= 0){
            p2->hp_actual = 0;
            p2->estado = 0;
            ganador = 1;
            duelo_activo = 0;
        }
    
    // Cambiamos el turno para que en la siguiente iteración le toque a P2
        turno = 0; 

        } else {
    // --- TURNO DE P2 ---
        dano2 = calcular_dano(p2, p1);
        p1->hp_actual -= dano2;
        
        if (p1->hp_actual <= 0) {
            p1->hp_actual = 0;
            p1->estado = 0;
            ganador = 2;
            duelo_activo = 0;
        }
        
        // Cambiamos el turno para que en la siguiente iteración le toque a P1
        turno = 1;
        }
    }

    return ganador;
}