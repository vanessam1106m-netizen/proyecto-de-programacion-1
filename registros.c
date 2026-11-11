typedef struct {
    int IU;
    char Especie[50];
    char Nombre[50];
    int Nivel;
    int Hp;
    int Ataque;
    int Defensa;
    int Velocidad;
    char Tipo[50];
    int Estado_actual;
} EjemplarPokemon;

typedef struct {
    int N_Pokedex;
    char Nombre_de_pokedex[50];
    char T_Primario[50];
    char T_Secundario[50];
    int Hp_base;
    int Ataque_base;
    int Defensa_base;
    int Velocidad_base;
} Pokedex;

typedef struct{
    int Identificador;
    char Nombre[50];
    char Equipo_Pokémon[50];
    int Cantidad_de_victorias;
    int Cantidad_de_empates;
    int Cantidad_de_derrotas;
    int Puntuación;
}Entrenador;