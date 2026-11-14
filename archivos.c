#include <stdio.h>
#include <string.h>

// arreglo con los nombres de tipos en el mismo orden del enum IdTipo (0 = Ninguno, 1 = Normal, etc)
char nombres_tipos[19][20] = {
    "Ninguno", "Normal", "Fuego", "Agua", "Planta", "Electrico", "Hielo",
    "Lucha", "Veneno", "Tierra", "Volador", "Psiquico", "Bicho",
    "Roca", "Fantasma", "Dragon", "Siniestro", "Acero", "Hada"
};

// funcion para saber el numero de tipo a partir de la palabra leida del archivo
int tipo_a_numero(char tipo[]) {
    int i;
    for (i = 1; i <= 18; i++) {
        if (strcmp(tipo, nombres_tipos[i]) == 0) {
            return i;
        }
    }
    return 0; // si no tiene segundo tipo o tiene guion "-"
}

// lee la pokedex oficial desde el archivo pokedex.txt
int cargar_pokedex(char ruta[], Pokedex catalogo[], int *total) {
    FILE *arch = fopen(ruta, "r");
    char linea[150];
    int cont = 0;

    if (arch == NULL) {
        printf("Aviso: no se encontro el archivo %s\n", ruta);
        return 0;
    }

    // lee linea por linea hasta llegar al final del archivo o a los 150
    while (fgets(linea, 150, arch) != NULL && cont < 150) {
        // si la linea es un comentario o un enter vacio, la saltamos
        if (linea[0] == '#' || linea[0] == '\n' || linea[0] == '\r') {
            continue;
        }

        Pokedex p;
        // leemos los datos de la especie
        if (sscanf(linea, "%d %s %s %s %d %d %d %d",
                   &p.N_Pokedex, p.Nombre_de_pokedex, p.T_Primario, p.T_Secundario,
                   &p.Hp_base, &p.Ataque_base, &p.Defensa_base, &p.Velocidad_base) == 8) {

            p.Valor_T_Primario = tipo_a_numero(p.T_Primario);

            if (strcmp(p.T_Secundario, "-") == 0) {
                strcpy(p.T_Secundario, "");
                p.Valor_T_Secundario = 0;
            } else {
                p.Valor_T_Secundario = tipo_a_numero(p.T_Secundario);
            }

            catalogo[cont] = p;
            cont++;
        }
    }

    fclose(arch);
    *total = cont;
    return 1;
}

// crea un ejemplar nuevo con sus estadisticas calculadas segun el nivel
void crear_ejemplar(Pokedex base, int nivel, EjemplarPokemon *poke, int *secuencia_iu) {
    poke->IU = *secuencia_iu;
    (*secuencia_iu)++;

    strcpy(poke->Especie, base.Nombre_de_pokedex);
    strcpy(poke->Nombre, base.Nombre_de_pokedex);
    poke->Nivel = nivel;

    // formula para calcular la vida y los atributos con el nivel
    poke->Hp = base.Hp_base + (nivel * 2);
    poke->Ataque = base.Ataque_base + nivel;
    poke->Defensa = base.Defensa_base + nivel;
    poke->Velocidad = base.Velocidad_base + nivel;

    poke->Valor_T_Primario = base.Valor_T_Primario;
    poke->Valor_T_Secundario = base.Valor_T_Secundario;
    poke->Estado_actual = 1; // 1 = vivo
}

// guarda los entrenadores y sus pokemones en entrenadores.txt
int guardar_entrenadores(char ruta[], Entrenador lista[], EjemplarPokemon equipos[][6], int cantidad_pokes[], int total) {
    FILE *arch = fopen(ruta, "w");
    int i, j;

    if (arch == NULL) {
        printf("Error al abrir %s para guardar.\n", ruta);
        return 0;
    }

    for (i = 0; i < total; i++) {
        // datos del entrenador
        fprintf(arch, "ENTRENADOR|%d|%s|%d|%d|%d|%d|%d\n",
                lista[i].Identificador,
                lista[i].Nombre,
                cantidad_pokes[i],
                lista[i].Cantidad_de_victorias,
                lista[i].Cantidad_de_empates,
                lista[i].Cantidad_de_derrotas,
                lista[i].Puntuacion);

        // cada pokemon de su equipo
        for (j = 0; j < cantidad_pokes[i]; j++) {
            EjemplarPokemon *p = &equipos[i][j];
            fprintf(arch, "POKEMON|%d|%s|%s|%d|%d|%d|%d|%d|%d|%d\n",
                    p->IU,
                    p->Especie,
                    p->Nombre,
                    p->Nivel,
                    p->Hp,
                    p->Ataque,
                    p->Defensa,
                    p->Velocidad,
                    p->Valor_T_Primario,
                    p->Valor_T_Secundario);
        }
    }

    fclose(arch);
    return 1;
}

// carga los entrenadores guardados de la sesion anterior
int cargar_entrenadores(char ruta[], Entrenador lista[], EjemplarPokemon equipos[][6], int cantidad_pokes[], int *total, int *secuencia_iu) {
    FILE *arch = fopen(ruta, "r");
    char linea[150];
    char cabecera[20];
    int cont = 0;
    int pos = -1;

    if (arch == NULL) {
        return 0; // si todavia no existe el archivo no pasa nada
    }

    while (fgets(linea, 150, arch) != NULL) {
        // miramos de que tipo es la linea
        sscanf(linea, "%19[^|]", cabecera);

        if (strcmp(cabecera, "ENTRENADOR") == 0) {
            pos = cont;
            cont++;
            cantidad_pokes[pos] = 0;

            sscanf(linea, "ENTRENADOR|%d|%[^|]|%d|%d|%d|%d|%d",
                   &lista[pos].Identificador,
                   lista[pos].Nombre,
                   &cantidad_pokes[pos],
                   &lista[pos].Cantidad_de_victorias,
                   &lista[pos].Cantidad_de_empates,
                   &lista[pos].Cantidad_de_derrotas,
                   &lista[pos].Puntuacion);

            // ponemos en cero para ir metiendo los pokemones uno a uno
            cantidad_pokes[pos] = 0;

        } else if (strcmp(cabecera, "POKEMON") == 0 && pos >= 0) {
            if (cantidad_pokes[pos] < 6) {
                EjemplarPokemon *p = &equipos[pos][cantidad_pokes[pos]];

                sscanf(linea, "POKEMON|%d|%[^|]|%[^|]|%d|%d|%d|%d|%d|%d|%d",
                       &p->IU,
                       p->Especie,
                       p->Nombre,
                       &p->Nivel,
                       &p->Hp,
                       &p->Ataque,
                       &p->Defensa,
                       &p->Velocidad,
                       &p->Valor_T_Primario,
                       &p->Valor_T_Secundario);

                p->Estado_actual = 1;

                // actualizamos el contador para que ningun IU nuevo se repita
                if (p->IU >= *secuencia_iu) {
                    *secuencia_iu = p->IU + 1;
                }

                cantidad_pokes[pos]++;
            }
        }
    }

    fclose(arch);
    *total = cont;
    return 1;
}
