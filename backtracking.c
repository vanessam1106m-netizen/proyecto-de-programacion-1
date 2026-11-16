#ifndef BACKTRACKING_C
#define BACKTRACKING_C

// estructura con los filtros que pide el usuario
typedef struct {
    int cantidad_pokemon;
    int nivel_max_total;
    int tipos_minimos;
    int hp_min_total;
    int ataque_min_total;
    int defensa_min_total;
    int velocidad_min_total;
    int no_repetir_especie;
} Restricciones;

// cuenta los tipos elementales diferentes que tiene el equipo
int contar_tipos(EjemplarPokemon equipo[], int n) {
    int tipos[18];
    int total = 0;
    int i, j;

    for (i = 0; i < n; i++) {
        // primer tipo
        if (equipo[i].Valor_T_Primario > 0) {
            int repetido = 0;
            for (j = 0; j < total; j++) {
                if (tipos[j] == equipo[i].Valor_T_Primario) {
                    repetido = 1;
                    break;
                }
            }
            if (!repetido && total < 18) {
                tipos[total] = equipo[i].Valor_T_Primario;
                total++;
            }
        }

        // segundo tipo si tiene
        if (equipo[i].Valor_T_Secundario > 0) {
            int repetido = 0;
            for (j = 0; j < total; j++) {
                if (tipos[j] == equipo[i].Valor_T_Secundario) {
                    repetido = 1;
                    break;
                }
            }
            if (!repetido && total < 18) {
                tipos[total] = equipo[i].Valor_T_Secundario;
                total++;
            }
        }
    }

    return total;
}

// revisa si el equipo que se esta armando no rompe ninguna regla
int es_valido(EjemplarPokemon equipo[], int n, Restricciones r) {
    int i, j;
    int suma_nivel = 0;
    int suma_hp = 0;
    int suma_atk = 0;
    int suma_def = 0;
    int suma_vel = 0;

    for (i = 0; i < n; i++) {
        suma_nivel += equipo[i].Nivel;
        suma_hp += equipo[i].Hp;
        suma_atk += equipo[i].Ataque;
        suma_def += equipo[i].Defensa;
        suma_vel += equipo[i].Velocidad;

        // si no se permite repetir la misma especie en el equipo
        if (r.no_repetir_especie) {
            for (j = i + 1; j < n; j++) {
                if (strcmp(equipo[i].Especie, equipo[j].Especie) == 0) {
                    return 0; // especie repetida
                }
            }
        }
    }

    // si ya nos pasamos del nivel maximo permitido, cortamos de una vez
    if (r.nivel_max_total > 0 && suma_nivel > r.nivel_max_total) {
        return 0;
    }

    // si ya se completaron los integrantes pedidos, verificar el resto de condiciones
    if (n == r.cantidad_pokemon) {
        if (r.tipos_minimos > 0 && contar_tipos(equipo, n) < r.tipos_minimos) {
            return 0;
        }
        if (r.hp_min_total > 0 && suma_hp < r.hp_min_total) {
            return 0;
        }
        if (r.ataque_min_total > 0 && suma_atk < r.ataque_min_total) {
            return 0;
        }
        if (r.defensa_min_total > 0 && suma_def < r.defensa_min_total) {
            return 0;
        }
        if (r.velocidad_min_total > 0 && suma_vel < r.velocidad_min_total) {
            return 0;
        }
    }

    return 1;
}

// busqueda recursiva con retroceso para armar el equipo
int buscar_equipo_bt(EjemplarPokemon candidatos[], int total_candidatos, int inicio,
                     EjemplarPokemon equipo[], int paso, Restricciones r) {
    int i;

    // si ya llegamos a la cantidad de integrantes que pidio el usuario
    if (paso == r.cantidad_pokemon) {
        return es_valido(equipo, paso, r);
    }

    // si los candidatos que quedan no alcanzan para llenar los puestos vacios
    if ((total_candidatos - inicio) < (r.cantidad_pokemon - paso)) {
        return 0;
    }

    // probar con cada candidato restante
    for (i = inicio; i < total_candidatos; i++) {
        equipo[paso] = candidatos[i];

        // revisamos si agregar a este candidato no rompe las reglas
        if (es_valido(equipo, paso + 1, r)) {
            // llamada recursiva para llenar el siguiente puesto
            if (buscar_equipo_bt(candidatos, total_candidatos, i + 1, equipo, paso + 1, r)) {
                return 1; // encontramos solucion
            }
        }
        // si no sirvio, el ciclo sigue y prueba con el proximo (retroceso)
    }

    return 0; // no hubo combinacion valida por aqui
}

#endif // BACKTRACKING_C
