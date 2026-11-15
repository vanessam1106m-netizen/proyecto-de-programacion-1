#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>


// asigna un numero entero fijo a cada tipo elemental (del 1 al 18) y 0 cuando un pokemon no tiene segundo tipo
// esto permite escribir palabras claras como FUEGO o AGUA en el codigo mientras el programa trabaja internamente con numeros
typedef enum {
    NINGUNO = 0,
    NORMAL = 1,   FUEGO = 2,     AGUA = 3,      PLANTA = 4,    ELECTRICO = 5, HIELO = 6,
    LUCHA = 7,    VENENO = 8,    TIERRA = 9,    VOLADOR = 10,  PSIQUICO = 11, BICHO = 12,
    ROCA = 13,    FANTASMA = 14, DRAGON = 15,   SINIESTRO = 16, ACERO = 17,   HADA = 18
} IdTipo;

// estructura compacta que reemplaza la matriz tradicional de 18x18 (324 casillas)
// guarda unicamente las 120 excepciones de dano y descarta los 204 cruces que dan dano normal (x1.0) para ahorrar memoria
typedef struct {
    int muy_efectivo[51];  // guarda los 51 codigos que duplican el dano (x2.0)
    int poco_efectivo[61]; // guarda los 61 codigos que reducen el dano a la mitad (x0.5)
    int sin_efecto[8];     // guarda los 8 codigos donde el defensor es inmune (x0.0)
} TablaEfectividad;

// base de datos constante con los 120 codigos calculados con la formula: (tipo_atacante * 100) + tipo_defensor
// todos los numeros estan ordenados de menor a mayor para poder encontrarlos rapido con busqueda binaria
const TablaEfectividad tabla_codigos = {
    .muy_efectivo = {
        204, 206, 212, 217,                // fuego (2) contra planta, hielo, bicho, acero
        302, 309, 313,                     // agua (3) contra fuego, tierra, roca
        403, 409, 413,                     // planta (4) contra agua, tierra, roca
        503, 510,                          // electrico (5) contra agua, volador
        604, 609, 610, 615,                // hielo (6) contra planta, tierra, volador, dragon
        701, 706, 713, 716, 717,           // lucha (7) contra normal, hielo, roca, siniestro, acero
        804, 818,                          // veneno (8) contra planta, hada
        902, 905, 908, 913, 917,           // tierra (9) contra fuego, electrico, veneno, roca, acero
        1004, 1007, 1012,                  // volador (10) contra planta, lucha, bicho
        1107, 1108,                        // psiquico (11) contra lucha, veneno
        1204, 1211, 1216,                  // bicho (12) contra planta, psiquico, siniestro
        1302, 1306, 1310, 1312,            // roca (13) contra fuego, hielo, volador, bicho
        1411, 1414,                        // fantasma (14) contra psiquico, fantasma
        1515,                              // dragon (15) contra dragon
        1611, 1614,                        // siniestro (16) contra psiquico, fantasma
        1706, 1713, 1718,                  // acero (17) contra hielo, roca, hada
        1807, 1815, 1816                   // hada (18) contra lucha, dragon, siniestro
    },

    .poco_efectivo = {
        113, 117,                          // normal (1) contra roca, acero
        202, 203, 213, 215,                // fuego (2) contra fuego, agua, roca, dragon
        303, 304, 315,                     // agua (3) contra agua, planta, dragon
        402, 404, 408, 410, 412, 415, 417, // planta (4) contra fuego, planta, veneno, volador, bicho, dragon, acero
        504, 505, 515,                     // electrico (5) contra planta, electrico, dragon
        602, 603, 606, 617,                // hielo (6) contra fuego, agua, hielo, acero
        708, 710, 711, 712, 718,           // lucha (7) contra veneno, volador, psiquico, bicho, hada
        808, 809, 813, 814,                // veneno (8) contra veneno, tierra, roca, fantasma
        904, 912,                          // tierra (9) contra planta, bicho
        1005, 1013, 1017,                  // volador (10) contra electrico, roca, acero
        1111, 1117,                        // psiquico (11) contra psiquico, acero
        1202, 1207, 1208, 1210, 1214, 1217, 1218, // bicho (12) contra fuego, lucha, veneno, volador, fantasma, acero, hada
        1307, 1309, 1317,                  // roca (13) contra lucha, tierra, acero
        1416,                              // fantasma (14) contra siniestro
        1517,                              // dragon (15) contra acero
        1607, 1616, 1618,                  // siniestro (16) contra lucha, siniestro, hada
        1702, 1703, 1705, 1717,            // acero (17) contra fuego, agua, electrico, acero
        1802, 1808, 1817                   // hada (18) contra fuego, veneno, acero
    },

    .sin_efecto = {
        // normal->fantasma(114), electrico->tierra(509), lucha->fantasma(714), veneno->acero(817),
        // tierra->volador(910), psiquico->siniestro(1116), fantasma->normal(1401), dragon->hada(1518)
        114, 509, 714, 817, 910, 1116, 1401, 1518
    }
};

// representa a un pokemon individual real que pertenece a un entrenador y entra a pelear en el duelo
// nota: aqui no se guardan los nombres de los tipos en char[] porque el motor de combate solo necesita sus numeros (int)
// para calcular la formula (atacante*100 + defensor), ahorrando memoria y evitando repetir texto que ya esta en la pokedex
typedef struct {
    int IU;                 // identificador unico de este ejemplar
    char Especie[50];       // nombre de la especie a la que pertenece (ej: "Charmander")
    char Nombre[50];        // apodo individual del pokemon (ej: "Charmi")
    int Nivel;              // nivel actual del ejemplar (se usa en la formula de dano)
    int Hp;                 // puntos de vida actuales que van bajando durante el combate
    int Ataque;             // puntos de ataque del ejemplar
    int Defensa;            // puntos de defensa del ejemplar
    int Velocidad;          // puntos de velocidad para decidir quien ataca primero
    int Valor_T_Primario;   // numero del tipo primario del 1 al 18 (segun enum IdTipo)
    int Valor_T_Secundario; // numero del tipo secundario del 1 al 18 (o 0 = NINGUNO si solo tiene un tipo)
    int Estado_actual;      // estado en el duelo: 1 = activo con vida, 0 = debilitado sin vida
} EjemplarPokemon;

// representa el catalogo general (enciclopedia de especies) con los datos de fabrica de cada pokemon
// aqui si se guardan los tipos en texto (char) ademas de su numero para poder leerlos de archivos o mostrarlos en pantalla,
// pero esta estructura nunca entra a pelear ni pierde vida, solo sirve como molde de consulta
typedef struct {
    int N_Pokedex;              // numero oficial de la especie en el catalogo
    char Nombre_de_pokedex[50]; // nombre oficial de la especie
    char T_Primario[50];        // nombre en texto del tipo primario (ej: "Fuego") para lectura o impresion
    int Valor_T_Primario;       // id numerico del tipo primario (del 1 al 18) que se le copiara al ejemplar
    char T_Secundario[50];      // nombre en texto del tipo secundario para lectura o impresion
    int Valor_T_Secundario;     // id numerico del tipo secundario (del 1 al 18, o 0 si no tiene)
    int Hp_base;                // vida base de fabrica de la especie
    int Ataque_base;            // ataque base de fabrica de la especie
    int Defensa_base;           // defensa base de fabrica de la especie
    int Velocidad_base;         // velocidad base de fabrica de la especie
} Pokedex;

// almacena la informacion de cada participante del torneo, su equipo y su historial de combates
typedef struct {
    int Identificador;          // numero unico del entrenador
    char Nombre[50];            // nombre del entrenador
    char Equipo_Pokemon[50];    // referencia o listado de los pokemon de su equipo
    int Cantidad_de_victorias;  // total de duelos ganados
    int Cantidad_de_empates;    // total de duelos empatados
    int Cantidad_de_derrotas;   // total de duelos perdidos
    int Puntuacion;             // puntaje total acumulado en el torneo
} Entrenador;


int busqueda_binaria(const int arreglo[], int tamano, int codigo_buscado);
float evaluar_un_tipo(int tipoAtacante, int tipoDefensor);
float obtener_multiplicador_tipos(int tipoAtacante1, int tipoAtacante2, int tipoDefensor1, int tipoDefensor2);
int calcular_dano(EjemplarPokemon *atacante, EjemplarPokemon *defensor);
int simular_combate_1v1(EjemplarPokemon *p1, EjemplarPokemon *p2);

int prueba_combate_1v1 (){
    // inicializa la semilla de numeros aleatorios usando el reloj actual para el desempate de velocidad
    srand(time(NULL));

// prueba 5: misma velocidad (100 vs 100) y tipos neutros (1.0f)
    EjemplarPokemon p1 = {1, "Raichu", "Rayo", 10, 50, 10, 20, 100, ELECTRICO, NINGUNO, 1};
    EjemplarPokemon p2 = {2, "Arcanine", "Fuego", 10, 50, 10, 20, 100, FUEGO, NINGUNO, 1};

    // muestra en pantalla el encabezado y la vida inicial de cada pokemon antes de pelear
    printf("=== INICIO DEL DUELO ===\n");
    printf("P1 %s (HP: %d) vs P2 %s (HP: %d)\n\n", p1.Nombre, p1.Hp, p2.Nombre, p2.Hp);

    // llama a la funcion del combate enviando las direcciones de memoria (&p1 y &p2) para que modifique sus vidas reales
    int ganador = simular_combate_1v1(&p1, &p2);

    // muestra el cierre del combate y evalua quien gano segun el numero retornado (1, 2 o 0)
    printf("=== FIN DEL DUELO ===\n");
    
    // si retorno 1, gano el ejemplar 1 y muestra con cuanta vida quedo
    if (ganador == 1) {
        printf("¡El Ganador es el Ejemplar 1 (%s)! HP final: %d\n", p1.Nombre, p1.Hp);
        
    // si retorno 2, gano el ejemplar 2 y muestra con cuanta vida quedo
    } else if (ganador == 2) {
        printf("¡El Ganador es el Ejemplar 2 (%s)! HP final: %d\n", p2.Nombre, p2.Hp);
        
    // si retorno 0, significa que ninguno pudo ganar (empate por vida en 0 o inmunidad mutua)
    } else {
        printf("¡El duelo termino en empate!\n");
    }

    // indica al sistema operativo que el programa finalizo correctamente
    return 0;
}


// recibe la lista ordenada, su cantidad de elementos y el codigo (atacante*100 + defensor) a buscar
int busqueda_binaria(const int arreglo[], int tamano, int codigo_buscado) {
    // inicio: primera posicion (0) | fin: ultima posicion (tamano - 1)
    // medio: posicion central actual | encontrado: bandera (0 = no esta, 1 = si esta)
    int inicio = 0, medio, fin = tamano - 1, encontrado = 0;

    while(inicio <= fin && !encontrado){// se repite mientras queden elementos por revisar (inicio <= fin) y no hayamos hallado el codigo (!encontrado)
        medio = (inicio + fin) / 2;// calcula el indice justo a la mitad del tramo actual
        
        if(arreglo[medio] == codigo_buscado){// caso 1: el numero en la mitad es exactamente el codigo que buscamos
            encontrado = 1; // activamos la bandera para salir del ciclo en la siguiente evaluacion
        } else if(arreglo[medio] < codigo_buscado){// caso 2: el numero de la mitad es menor al buscado (como esta ordenado, el nuestro debe estar a la derecha)
            inicio = medio + 1; // movemos el inicio despues de la mitad, descartando toda la parte izquierda
        } else{// caso 3: el numero de la mitad es mayor al buscado (el nuestro debe estar a la izquierda)
            fin = medio - 1;    // movemos el fin antes de la mitad, descartando toda la parte derecha
        }

    }
    return encontrado;// devuelve el estado final de la bandera: 1 si lo encontro o 0 si reviso todo y no existe
}


// recibe el id numerico del tipo que ataca y de uno de los tipos del que defiende (del 1 al 18, o 0 si no tiene)
float evaluar_un_tipo(int tipoAtacante, int tipoDefensor) {
    float multiplicador = 1.0f;  // asume por defecto que el ataque es normal (x1.0) si no aparece en ninguna de las 3 listas
    int encontrado = 0, codigo;// encontrado: bandera en 0 para cortar la busqueda cuando halle el codigo | codigo: guarda la combinacion

        if(tipoDefensor > 0){ // verifica que el defensor tenga ese tipo (si es 0 significa que no tiene segundo tipo y se salta todo)
            codigo = (tipoAtacante * 100) + tipoDefensor;// genera el numero unico de la pelea (ej: atacante 2 y defensor 4 se convierte en 204)
            
            if(busqueda_binaria(tabla_codigos.poco_efectivo, 61, codigo)){// paso 1: busca primero en poco_efectivo (61 elementos) por ser la lista con mas combinaciones
                multiplicador = 0.5f; // si esta en la lista, el dano se reduce a la mitad
                encontrado = 1;       // activa la bandera para no buscar en las listas de abajo
            }

            if(!encontrado){// paso 2: solo entra si la bandera sigue en 0 (es decir, no estaba en poco_efectivo)
                // busca el codigo en la lista de muy_efectivo (51 elementos)
                if(busqueda_binaria(tabla_codigos.muy_efectivo, 51, codigo)){
                    multiplicador = 2.0f; // si esta en la lista, el dano se duplica
                    encontrado = 1;       // activa la bandera para saltarse la ultima lista
                }
            }

            if(!encontrado){// paso 3: solo entra si la bandera sigue en 0 (no estaba ni en poco_efectivo ni en muy_efectivo)
                // busca el codigo en la lista de sin_efecto (8 elementos)
                if(busqueda_binaria(tabla_codigos.sin_efecto, 8, codigo)){
                    multiplicador = 0.0f; // si esta en la lista, el defensor es inmune y el dano es cero
                    encontrado = 1;       // activa la bandera indicando que ya se encontro
                }
            }
    }
    return multiplicador;// devuelve el multiplicador hallado (0.5, 2.0, 0.0) o 1.0 si no entro en ninguna lista
}


// recibe los 2 tipos del atacante y los 2 del defensor (si alguno no tiene 2do tipo, recibe 0)
float obtener_multiplicador_tipos(int tipoAtacante1, int tipoAtacante2, int tipoDefensor1, int tipoDefensor2){
    
    // todas las variables inician en 1.0f (neutro) por si algun pokemon solo tiene un tipo
    float m1 = 1.0f, m2 = 1.0f, m3 = 1.0f, m4 = 1.0f;

    // 1. evalua el tipo primario del atacante contra el primario del defensor
    m1 = evaluar_un_tipo(tipoAtacante1, tipoDefensor1);

    // 2. si el defensor tiene segundo tipo, lo evalua contra el primario del atacante
    if(tipoDefensor2 > 0 && tipoDefensor2 != tipoDefensor1){
        m2 = evaluar_un_tipo(tipoAtacante1, tipoDefensor2);
    }

    // 3. si el atacante tiene segundo tipo (no es 0), lo evalua contra el defensor
    if(tipoAtacante2 > 0 && tipoAtacante2 != tipoAtacante1){
        
        // secundario del atacante contra primario del defensor
        m3 = evaluar_un_tipo(tipoAtacante2, tipoDefensor1);

        // si el defensor tambien tiene segundo tipo (caso 2 vs 2), evalua secundario contra secundario
        if(tipoDefensor2 > 0 && tipoDefensor2 != tipoDefensor1){
            m4 = evaluar_un_tipo(tipoAtacante2, tipoDefensor2);
        }
    }

    // multiplica los resultados (los que no se usaron quedaron en 1.0f y no alteran nada)
    return m1 * m2 * m3 * m4;
}


// recibe los punteros del pokemon que ataca y del que defiende para leer sus estadisticas y devolver el dano total
int calcular_dano(EjemplarPokemon *atacante, EjemplarPokemon *defensor){
    int dano_base, dano_final;// dano_base: golpe inicial sin tipos | dano_final: puntos enteros de vida que se restaran al defensor
    float multiplicador_tipos, dano_calculado;// multiplicador_tipos: factor de ventaja/desventaja (ej: 0.5, 2.0) | dano_calculado: dano con decimales
    
        // 1. aplica la formula de combate: multiplica el nivel por el ataque del agresor y le resta la defensa del rival
        dano_base = (atacante->Nivel * atacante->Ataque) - defensor->Defensa;
        
        // 2. filtro de seguridad: si la defensa del rival era tan alta que el calculo dio 0 o negativo, fija el dano base en 2
        if(dano_base <= 0){
            dano_base = 2;
        }
        
        // 3. busca la efectividad combinada enviando los dos tipos del atacante y los dos del defensor
        multiplicador_tipos = obtener_multiplicador_tipos(atacante->Valor_T_Primario, atacante->Valor_T_Secundario, defensor->Valor_T_Primario, defensor->Valor_T_Secundario);
        
        // multiplica el golpe base por el factor de tipos (puede dar decimales si el factor fue 0.5 o 0.25)
        dano_calculado = (dano_base * multiplicador_tipos);
        
        // redondea el resultado con decimales al numero entero mas cercano y lo convierte a int
        dano_final = (int)round(dano_calculado);
        
        // 4. segundo filtro de seguridad: si el ataque no era inmune (multiplicador > 0) pero al dividir dio menos de 1, asegura 2 de dano
        if(dano_final < 1 && multiplicador_tipos > 0){
            dano_final = 2;
        }
        
    
    return dano_final;// devuelve la cantidad exacta de puntos de vida (hp) que perdera el defensor en este turno
}


// simula la pelea por turnos entre dos ejemplares y devuelve: 1 si gana p1, 2 si gana p2, o 0 si hay empate
int simular_combate_1v1(EjemplarPokemon *p1, EjemplarPokemon *p2){
    
    // duelo_activo: bandera del while (1 = siguen peleando, 0 = termino la pelea)
    // dano1 y dano2 inician en -1 para saber cuando ambos ya intentaron atacar al menos una vez
    int duelo_activo = 1, dano1 = -1, dano2 = -1;
    
    // ganador: guarda el resultado final (0 = empate, 1 = p1, 2 = p2) | turno: 1 ataca p1, 0 ataca p2
    int ganador = 0, turno;

    // 1. verificar si alguno (o ambos) ingreso al combate ya sin puntos de vida (hp <= 0)
    if (p1->Hp <= 0 && p2->Hp <= 0) {
        duelo_activo = 0; // se cancela el combate porque ninguno puede pelear
        ganador = 0;      // retorna 0 indicando empate
    } else if (p1->Hp <= 0) {
        duelo_activo = 0; // no hay pelea porque p1 ya entro debilitado
        ganador = 2;      // gana p2 automaticamente
    } else if (p2->Hp <= 0) {
        duelo_activo = 0; // no hay pelea porque p2 ya entro debilitado
        ganador = 1;      // gana p1 automaticamente
    }

    // 2. determinar quien ataca primero en la ronda inicial comparando la velocidad de ambos
    if(duelo_activo){
        // si p1 es mas rapido que p2, empieza atacando p1 (turno = 1)
        if(p1->Velocidad > p2->Velocidad){
            turno = 1;
        // si p2 es mas rapido que p1, empieza atacando p2 (turno = 0)
        } else if(p2->Velocidad > p1->Velocidad){
            turno = 0;
        // si tienen exactamente la misma velocidad, se decide al azar (rand() % 2 da 0 o 1)
        } else{
            turno = rand() % 2;
        }
    }

    // 3. ciclo principal del combate: se repite turno a turno mientras la bandera duelo_activo sea 1
    while(duelo_activo){

        // si turno vale 1, le toca atacar al ejemplar 1 (p1)
        if(turno == 1){
            
            // calcula cuantos puntos de vida le quita p1 a p2
            dano1 = calcular_dano(p1, p2);
            
            // le resta ese dano a la vida actual de p2
            p2->Hp -= dano1;
            
            // revisa si p2 se quedo sin vida tras recibir el golpe
            if(p2->Hp <= 0){
                p2->Hp = 0;            // ajusta la vida a 0 para que no quede un numero negativo
                p2->Estado_actual = 0; // cambia el estado de p2 a 0 (debilitado)
                ganador = 1;           // declara a p1 como ganador del duelo
                duelo_activo = 0;      // apaga la bandera para terminar el ciclo while
            }
        
            // cambia el turno a 0 para que en la siguiente vuelta del while ataque p2
            turno = 0; 

        // si turno vale 0, le toca atacar al ejemplar 2 (p2)
        } else {
            
            // calcula cuantos puntos de vida le quita p2 a p1
            dano2 = calcular_dano(p2, p1);
            
            // le resta ese dano a la vida actual de p1
            p1->Hp -= dano2;
            
            // revisa si p1 se quedo sin vida tras recibir el golpe
            if (p1->Hp <= 0) {
                p1->Hp = 0;            // ajusta la vida a 0 para evitar numeros negativos
                p1->Estado_actual = 0; // cambia el estado de p1 a 0 (debilitado)
                ganador = 2;           // declara a p2 como ganador del duelo
                duelo_activo = 0;      // apaga la bandera para terminar el ciclo while
            }
            
            // cambia el turno a 1 para que en la siguiente vuelta del while ataque p1
            turno = 1;
        }

        // proteccion contra bucle infinito: si ambos ya atacaron y los dos hacen 0 de dano (inmunes entre si)
        if(dano1 == 0 && dano2 == 0){
            ganador = 0;      // ninguno puede hacerse dano nunca, se declara empate
            duelo_activo = 0; // apaga la bandera para salir del while
        }
    }

    // devuelve 1 si gano p1, 2 si gano p2, o 0 si empataron
    return ganador;
}


// ============================================================================
// ====================== INICIO DE MI PARTE (PUNTOS 4 Y 5) ===================
// ============================================================================
// aqui se agrega la logica del torneo (grupos, puntuacion, eliminatorias, carga de resultados)
// y el menu principal con control de errores. no se toco nada de lo anterior
// (estructuras, tabla de tipos y motor de combate), solo se agrego codigo nuevo

#include<string.h>

// activa las versiones provisionales de las opciones 1 a 5 del menu (pokedex, registrar entrenador, equipos...)
// cuando tus companeros ya tengan sus funciones reales, borra o comenta esta linea
#define USAR_FUNCIONES_PROVISIONALES

// ---------------------------------------------------------------------------
// constantes del torneo
// ---------------------------------------------------------------------------
#define MAX_ENTRENADORES 32      // 8 grupos x 4 entrenadores
#define TAM_GRUPO 4              // entrenadores por grupo
#define MAX_GRUPOS 8             // grupos A..H (16 clasificados)
#define COMBATES_POR_GRUPO 6     // todos contra todos en un grupo de 4
#define COMBATES_GRUPOS 48       // combates 1 al 48 (fase de grupos)
#define COMBATES_TOTALES 64      // combates 49 al 64 son las eliminatorias
#define POKEMON_POR_EQUIPO 6     // maximo de pokemon rivales que se pueden derrotar en un combate
#define MAX_LINEAS_ARCHIVO 200   // maximo de lineas que se leen de un archivo de resultados
#define ARCHIVO_HISTORIAL "historial.txt"
#define ARCHIVO_CLASIFICACION "clasificacion.txt"
#define ARCHIVO_ENTRENADORES "entrenadores.txt"

// guarda el estado de un combate del torneo (los 64 combates numerados del reglamento)
typedef struct {
    int numero;     // numero del combate (1 al 64)
    int id1;        // identificador del entrenador 1 (0 = todavia por definir)
    int id2;        // identificador del entrenador 2 (0 = todavia por definir)
    int der1;       // pokemon rivales derrotados por el entrenador 1
    int der2;       // pokemon rivales derrotados por el entrenador 2
    int jugado;     // 0 = pendiente, 1 = ya tiene resultado
    int ganador;    // identificador del ganador (0 = empate o sin jugar)
    int perdedor;   // identificador del perdedor (0 = empate o sin jugar)
} Combate;

// acumulado de un entrenador en la fase de grupos (se recalcula desde los combates jugados)
typedef struct {
    int puntos;
    int victorias;
    int empates;
    int derrotas;
    int derrotados;  // pokemon rivales derrotados en toda la fase de grupos (3er criterio de desempate)
    int jugados;
} Estadistica;

// ---------------------------------------------------------------------------
// datos globales del torneo
// ---------------------------------------------------------------------------
Entrenador entrenadores[MAX_ENTRENADORES];   // entrenadores registrados
int total_entrenadores = 0;                  // cuantos hay registrados
Estadistica estadisticas[MAX_ENTRENADORES];  // misma posicion que el arreglo entrenadores
Combate combates[COMBATES_TOTALES + 1];      // se usan las posiciones 1 al 64 (la 0 no se usa)
int miembros_grupo[MAX_GRUPOS][TAM_GRUPO];   // posiciones (en entrenadores) de cada integrante del grupo
int orden_grupo[MAX_GRUPOS][TAM_GRUPO];      // lo mismo pero ordenado de 1ro a 4to segun la clasificacion
int historial[COMBATES_TOTALES];             // numeros de combate en el orden en que se registraron
int total_historial = 0;                     // cuantos resultados se han registrado
int grupos_generados = 0;                    // 1 cuando los grupos y enfrentamientos ya fueron armados
int total_grupos = 0;                        // cantidad de grupos armados (entrenadores / 4)
int entrenadores_en_grupos = 0;              // cuantos entrenadores habia cuando se armaron los grupos
int fin_de_entrada = 0;                      // pasa a 1 si se acaba la entrada de datos (Ctrl+D / Ctrl+Z)

const char *NOMBRES_GRUPO[MAX_GRUPOS] = {"Grupo A", "Grupo B", "Grupo C", "Grupo D", "Grupo E", "Grupo F", "Grupo G", "Grupo H"};

// los 6 enfrentamientos de un grupo, usando la posicion del entrenador dentro del grupo (0 a 3)
const int PARES_GRUPO[COMBATES_POR_GRUPO][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};

// cruces de octavos (combates 49 al 56): {grupo1, puesto1, grupo2, puesto2}
// grupo: 0 = A, 1 = B ... | puesto: 0 = primero, 1 = segundo
const int CRUCES_OCTAVOS[8][4] = {
    {0, 0, 1, 1},  // 49: 1A vs 2B
    {2, 0, 3, 1},  // 50: 1C vs 2D
    {4, 0, 5, 1},  // 51: 1E vs 2F
    {6, 0, 7, 1},  // 52: 1G vs 2H
    {1, 0, 0, 1},  // 53: 1B vs 2A
    {3, 0, 2, 1},  // 54: 1D vs 2C
    {5, 0, 4, 1},  // 55: 1F vs 2E
    {7, 0, 6, 1}   // 56: 1H vs 2G
};

// de donde sale cada participante de los combates 57 al 64
// numero positivo = ganador de ese combate | numero negativo = perdedor de ese combate
const int FUENTES_LLAVE[8][2] = {
    {49, 50},    // 57: cuartos
    {53, 54},    // 58: cuartos
    {51, 52},    // 59: cuartos
    {55, 56},    // 60: cuartos
    {57, 58},    // 61: semifinal
    {59, 60},    // 62: semifinal
    {-61, -62},  // 63: tercer lugar (perdedores de semifinales)
    {61, 62}     // 64: final (ganadores de semifinales)
};

// ---------------------------------------------------------------------------
// prototipos de mi parte
// ---------------------------------------------------------------------------
// entrada segura de datos
void limpiar_linea(char *texto);
int leer_entero(const char *mensaje, int minimo, int maximo);
void leer_texto(const char *mensaje, char *destino, int tamano);

// utilidades de busqueda y nombres
int buscar_entrenador(int id);
const char *nombre_de(int id);
int grupo_de_indice(int indice);
const char *nombre_ronda(int numero);

// 4.1 y 4.2: grupos, enfrentamientos, puntuacion y clasificacion
int preparar_torneo(void);
int jugados_grupo(int g);
int grupo_completo(int g);
void recalcular_estadisticas(void);
int es_mejor(int a, int b);
void ordenar_grupo(int g);
int obtener_clasificado(int g, int puesto);

// 4.3: eliminatorias
int participante_de(int codigo);
void armar_llaves(void);
void actualizar_torneo(void);
int torneo_terminado(void);

// 4.4: validacion, carga y persistencia
const char *eliminado_en(int id);
int decidir_ganador(int id1, int id2, int d1, int d2);
int registrar_resultado(int numero, int id1, int id2, int der1, int der2);
const char *texto_error(int codigo);
void anunciar_avance(int numero);
void cargar_resultado_teclado(void);
void cargar_resultados_archivo(const char *nombre);
int guardar_historial(const char *nombre);
int exportar_clasificacion(const char *nombre);

// impresion (sirven para pantalla o para archivo segun el FILE* que reciban)
void imprimir_combate(FILE *s, int n);
void imprimir_clasificacion(FILE *s);
void imprimir_podio(FILE *s);
void imprimir_enfrentamientos(int fase);
void imprimir_historial(FILE *s);

// 5: menu principal y opciones
void mostrar_menu(void);
void menu_cargar_resultados(void);
void consultar_clasificacion(void);
void consultar_enfrentamientos(void);
void consultar_historial(void);
void mostrar_resultados_globales(void);
void mostrar_campeon(void);
void menu_principal(void);

// opciones 1 a 5 del menu (las implementan otros integrantes; abajo hay versiones provisionales)
void consultar_pokedex(void);
void registrar_entrenador(void);
void crear_equipo(void);
void consultar_entrenadores(void);
void consultar_equipos(void);


// ===========================================================================
// ENTRADA SEGURA DE DATOS (control de errores del punto 5)
// ===========================================================================

// quita el salto de linea del final de un texto leido con fgets
void limpiar_linea(char *texto){
    int i = 0, fin = 0;

    while(texto[i] != '\0' && !fin){
        if(texto[i] == '\n' || texto[i] == '\r'){
            texto[i] = '\0'; // corta el texto justo donde estaba el salto de linea
            fin = 1;
        } else {
            i++;
        }
    }
}

// pide un numero entero entre minimo y maximo y repite la pregunta hasta que el usuario escriba algo valido
// asi, escribir letras, simbolos o numeros fuera de rango nunca cierra el programa
int leer_entero(const char *mensaje, int minimo, int maximo){
    char buffer[100];
    char *resto;
    long valor = 0;
    int valido = 0, resultado = minimo, leyo_numero, largo, descarte;

    while(!valido && !fin_de_entrada){
        printf("%s", mensaje);

        if(fgets(buffer, sizeof(buffer), stdin) == NULL){
            // se cerro la entrada (Ctrl+D o Ctrl+Z): se avisa con la bandera para que los ciclos terminen
            fin_de_entrada = 1;
        } else {
            largo = (int)strlen(buffer);

            // si la linea era mas larga que el buffer, se descarta lo que sobro para que no contamine la siguiente lectura
            if(largo == (int)sizeof(buffer) - 1 && buffer[largo - 1] != '\n'){
                do {
                    descarte = getchar();
                } while(descarte != '\n' && descarte != EOF);
                printf("Entrada demasiado larga. Intenta de nuevo.\n");
            } else {
                valor = strtol(buffer, &resto, 10);
                leyo_numero = (resto != buffer); // si resto == buffer, no se pudo leer ningun numero

                // ignora espacios al final (ej: "5  ")
                while(*resto == ' ' || *resto == '\t'){
                    resto++;
                }

                if(!leyo_numero || (*resto != '\n' && *resto != '\r' && *resto != '\0')){
                    printf("Entrada invalida: escribe solo un numero entero.\n");
                } else if(valor < minimo || valor > maximo){
                    printf("Valor fuera de rango: debe estar entre %d y %d.\n", minimo, maximo);
                } else {
                    resultado = (int)valor;
                    valido = 1;
                }
            }
        }
    }
    return resultado;
}

// pide un texto que no puede quedar vacio (se repite hasta que escriba algo)
void leer_texto(const char *mensaje, char *destino, int tamano){
    int valido = 0, largo, descarte;

    destino[0] = '\0';

    while(!valido && !fin_de_entrada){
        printf("%s", mensaje);

        if(fgets(destino, tamano, stdin) == NULL){
            destino[0] = '\0';
            fin_de_entrada = 1;
        } else {
            largo = (int)strlen(destino);

            // descarta el resto si el usuario escribio mas de lo que cabe
            if(largo == tamano - 1 && destino[largo - 1] != '\n'){
                do {
                    descarte = getchar();
                } while(descarte != '\n' && descarte != EOF);
            }

            limpiar_linea(destino);

            if(destino[0] == '\0'){
                printf("El texto no puede estar vacio.\n");
            } else {
                valido = 1;
            }
        }
    }
}


// ===========================================================================
// UTILIDADES
// ===========================================================================

// busca un entrenador por su identificador y devuelve su posicion en el arreglo (o -1 si no existe)
int buscar_entrenador(int id){
    int i = 0, indice = -1;

    while(i < total_entrenadores && indice < 0){
        if(entrenadores[i].Identificador == id){
            indice = i;
        }
        i++;
    }
    return indice;
}

// devuelve el nombre del entrenador con ese identificador ("Por definir" si el id es 0)
const char *nombre_de(int id){
    const char *nombre = "Por definir";
    int indice;

    if(id != 0){
        indice = buscar_entrenador(id);
        if(indice >= 0){
            nombre = entrenadores[indice].Nombre;
        } else {
            nombre = "Desconocido";
        }
    }
    return nombre;
}

// devuelve en que grupo (0 = A, 1 = B...) esta el entrenador de esa posicion, o -1 si no esta en ninguno
int grupo_de_indice(int indice){
    int g, j, grupo = -1;

    if(grupos_generados){
        for(g = 0; g < total_grupos; g++){
            for(j = 0; j < TAM_GRUPO; j++){
                if(miembros_grupo[g][j] == indice){
                    grupo = g;
                }
            }
        }
    }
    return grupo;
}

// devuelve el nombre de la ronda a la que pertenece un numero de combate
const char *nombre_ronda(int numero){
    const char *nombre = "Desconocida";

    if(numero >= 1 && numero <= COMBATES_GRUPOS){
        nombre = NOMBRES_GRUPO[(numero - 1) / COMBATES_POR_GRUPO];
    } else if(numero >= 49 && numero <= 56){
        nombre = "Octavos de final";
    } else if(numero >= 57 && numero <= 60){
        nombre = "Cuartos de final";
    } else if(numero == 61 || numero == 62){
        nombre = "Semifinal";
    } else if(numero == 63){
        nombre = "Tercer lugar";
    } else if(numero == 64){
        nombre = "Final";
    }
    return nombre;
}


// ===========================================================================
// 4.1 ORGANIZACION DE LA FASE DE GRUPOS Y CALENDARIO
// ===========================================================================

// arma los grupos (A, B, C...) de 4 entrenadores en el orden en que se registraron
// y genera los 6 enfrentamientos de cada grupo (combates 1 al 48)
// devuelve 1 si el torneo esta listo, 0 si todavia no se puede armar (y explica por que)
// mientras no haya resultados cargados, los grupos se vuelven a armar si cambio la cantidad de entrenadores
int preparar_torneo(void){
    int listo = 0, g, j, k, n;

    if(grupos_generados && (total_historial > 0 || entrenadores_en_grupos == total_entrenadores)){
        listo = 1; // ya estaban armados y siguen vigentes
    } else {
        grupos_generados = 0;

        if(total_entrenadores < TAM_GRUPO){
            printf("Se necesitan al menos %d entrenadores registrados para armar un grupo (hay %d).\n", TAM_GRUPO, total_entrenadores);
        } else if(total_entrenadores % TAM_GRUPO != 0){
            printf("La cantidad de entrenadores debe ser multiplo de %d para formar grupos completos (hay %d).\n", TAM_GRUPO, total_entrenadores);
        } else {
            total_grupos = total_entrenadores / TAM_GRUPO;
            entrenadores_en_grupos = total_entrenadores;

            // limpia todos los combates y deja el numero de cada uno
            for(n = 0; n <= COMBATES_TOTALES; n++){
                combates[n].numero = n;
                combates[n].id1 = 0;
                combates[n].id2 = 0;
                combates[n].der1 = 0;
                combates[n].der2 = 0;
                combates[n].jugado = 0;
                combates[n].ganador = 0;
                combates[n].perdedor = 0;
            }
            total_historial = 0;

            // reparte los entrenadores en grupos de 4 segun su orden de registro
            for(g = 0; g < total_grupos; g++){
                for(j = 0; j < TAM_GRUPO; j++){
                    miembros_grupo[g][j] = g * TAM_GRUPO + j;
                }
            }

            // genera los 6 enfrentamientos de cada grupo (todos contra todos, una sola vez)
            for(g = 0; g < total_grupos; g++){
                for(k = 0; k < COMBATES_POR_GRUPO; k++){
                    n = g * COMBATES_POR_GRUPO + k + 1;
                    combates[n].id1 = entrenadores[miembros_grupo[g][PARES_GRUPO[k][0]]].Identificador;
                    combates[n].id2 = entrenadores[miembros_grupo[g][PARES_GRUPO[k][1]]].Identificador;
                }
            }

            grupos_generados = 1;
            actualizar_torneo();
            listo = 1;

            printf("\nGrupos generados automaticamente:\n");
            for(g = 0; g < total_grupos; g++){
                printf("  %s: ", NOMBRES_GRUPO[g]);
                for(j = 0; j < TAM_GRUPO; j++){
                    printf("%s%s", entrenadores[miembros_grupo[g][j]].Nombre, (j < TAM_GRUPO - 1) ? ", " : "\n");
                }
            }
            if(total_grupos < MAX_GRUPOS){
                printf("Nota: la fase eliminatoria solo se habilita con %d entrenadores (%d grupos).\n", MAX_ENTRENADORES, MAX_GRUPOS);
            }
        }
    }
    return listo;
}


// ===========================================================================
// 4.2 SISTEMA DE PUNTUACION Y DESEMPATES
// ===========================================================================

// cuenta cuantos de los 6 combates de un grupo ya tienen resultado
int jugados_grupo(int g){
    int k, cantidad = 0;

    for(k = 1; k <= COMBATES_POR_GRUPO; k++){
        if(combates[g * COMBATES_POR_GRUPO + k].jugado){
            cantidad++;
        }
    }
    return cantidad;
}

// devuelve 1 si ya se jugaron los 6 combates del grupo
int grupo_completo(int g){
    return (jugados_grupo(g) == COMBATES_POR_GRUPO);
}

// vuelve a calcular puntos, victorias, empates, derrotas y pokemon derrotados desde cero
// leyendo todos los combates de grupo ya jugados (asi la tabla siempre es coherente)
// 3 puntos por victoria, 1 por empate, 0 por derrota
void recalcular_estadisticas(void){
    int i, n, a, b;

    memset(estadisticas, 0, sizeof(estadisticas));

    for(n = 1; n <= COMBATES_GRUPOS; n++){
        if(combates[n].jugado){
            a = buscar_entrenador(combates[n].id1);
            b = buscar_entrenador(combates[n].id2);

            estadisticas[a].jugados++;
            estadisticas[b].jugados++;
            estadisticas[a].derrotados += combates[n].der1;
            estadisticas[b].derrotados += combates[n].der2;

            if(combates[n].ganador == 0){
                estadisticas[a].empates++;
                estadisticas[b].empates++;
                estadisticas[a].puntos += 1;
                estadisticas[b].puntos += 1;
            } else if(combates[n].ganador == combates[n].id1){
                estadisticas[a].victorias++;
                estadisticas[a].puntos += 3;
                estadisticas[b].derrotas++;
            } else {
                estadisticas[b].victorias++;
                estadisticas[b].puntos += 3;
                estadisticas[a].derrotas++;
            }
        }
    }

    // copia los resultados a la estructura Entrenador (victorias, empates, derrotas y puntuacion de la fase de grupos)
    for(i = 0; i < total_entrenadores; i++){
        entrenadores[i].Cantidad_de_victorias = estadisticas[i].victorias;
        entrenadores[i].Cantidad_de_empates = estadisticas[i].empates;
        entrenadores[i].Cantidad_de_derrotas = estadisticas[i].derrotas;
        entrenadores[i].Puntuacion = estadisticas[i].puntos;
    }
}

// compara dos entrenadores (por su posicion en el arreglo) con la jerarquia oficial de desempate:
// 1) mas puntos  2) mas victorias  3) mas pokemon rivales derrotados
// devuelve 1 si 'a' va estrictamente por encima de 'b', y 0 si 'b' va arriba o si empatan en los 3 criterios
int es_mejor(int a, int b){
    int resultado = 0;

    if(estadisticas[a].puntos != estadisticas[b].puntos){
        resultado = (estadisticas[a].puntos > estadisticas[b].puntos);
    } else if(estadisticas[a].victorias != estadisticas[b].victorias){
        resultado = (estadisticas[a].victorias > estadisticas[b].victorias);
    } else if(estadisticas[a].derrotados != estadisticas[b].derrotados){
        resultado = (estadisticas[a].derrotados > estadisticas[b].derrotados);
    }
    return resultado;
}

// ordena los 4 integrantes de un grupo de mejor a peor (ordenamiento por insercion, estable:
// si empatan en los 3 criterios se mantiene el orden de registro)
void ordenar_grupo(int g){
    int i, j, temporal, mover;

    for(j = 0; j < TAM_GRUPO; j++){
        orden_grupo[g][j] = miembros_grupo[g][j];
    }

    for(i = 1; i < TAM_GRUPO; i++){
        temporal = orden_grupo[g][i];
        j = i - 1;
        mover = 1;

        while(j >= 0 && mover){
            if(es_mejor(temporal, orden_grupo[g][j])){
                orden_grupo[g][j + 1] = orden_grupo[g][j]; // corre al de la izquierda un lugar a la derecha
                j--;
            } else {
                mover = 0;
            }
        }
        orden_grupo[g][j + 1] = temporal;
    }
}

// devuelve el identificador del clasificado de un grupo (puesto 0 = primero, 1 = segundo)
// solo existe cuando el grupo ya jugo sus 6 combates; si no, devuelve 0
int obtener_clasificado(int g, int puesto){
    int id = 0;

    if(grupo_completo(g)){
        id = entrenadores[orden_grupo[g][puesto]].Identificador;
    }
    return id;
}


// ===========================================================================
// 4.3 FASE ELIMINATORIA (OCTAVOS, CUARTOS, SEMIFINALES, TERCER LUGAR Y FINAL)
// ===========================================================================

// codigo > 0 : devuelve el ganador de ese combate | codigo < 0 : devuelve el perdedor del combate (valor absoluto)
// devuelve 0 si ese combate todavia no se juega
int participante_de(int codigo){
    int n = (codigo < 0) ? -codigo : codigo;
    int id = 0;

    if(combates[n].jugado){
        id = (codigo > 0) ? combates[n].ganador : combates[n].perdedor;
    }
    return id;
}

// completa los participantes de los combates 49 al 64 en cuanto se conocen
// octavos: se arman cuando los dos grupos involucrados terminaron | resto: cuando se jugaron los combates anteriores
void armar_llaves(void){
    int i, n, g1, g2, c1, c2;

    if(total_grupos == MAX_GRUPOS){
        // octavos de final (combates 49 al 56)
        for(i = 0; i < 8; i++){
            n = 49 + i;
            if(combates[n].id1 == 0){
                g1 = CRUCES_OCTAVOS[i][0];
                g2 = CRUCES_OCTAVOS[i][2];

                if(grupo_completo(g1) && grupo_completo(g2)){
                    combates[n].id1 = obtener_clasificado(g1, CRUCES_OCTAVOS[i][1]);
                    combates[n].id2 = obtener_clasificado(g2, CRUCES_OCTAVOS[i][3]);
                }
            }
        }

        // cuartos, semifinales, tercer lugar y final (combates 57 al 64)
        for(i = 0; i < 8; i++){
            n = 57 + i;
            if(combates[n].id1 == 0){
                c1 = participante_de(FUENTES_LLAVE[i][0]);
                c2 = participante_de(FUENTES_LLAVE[i][1]);

                if(c1 != 0 && c2 != 0){
                    combates[n].id1 = c1;
                    combates[n].id2 = c2;
                }
            }
        }
    }
}

// refresca todo despues de cada resultado: tabla de posiciones, orden de los grupos y llaves eliminatorias
void actualizar_torneo(void){
    int g;

    recalcular_estadisticas();
    for(g = 0; g < total_grupos; g++){
        ordenar_grupo(g);
    }
    armar_llaves();
}

// devuelve 1 si ya se jugaron la final y el tercer puesto (el podio esta completo)
int torneo_terminado(void){
    return (combates[63].jugado && combates[64].jugado);
}


// ===========================================================================
// 4.4 VALIDACION, CARGA Y PERSISTENCIA DE RESULTADOS
// ===========================================================================

// si el entrenador ya quedo fuera del torneo devuelve el nombre de la ronda donde quedo eliminado, si no devuelve NULL
// (los perdedores de semifinal no cuentan como eliminados porque todavia juegan el tercer puesto)
const char *eliminado_en(int id){
    const char *ronda = NULL;
    int n, g, indice;

    for(n = 49; n <= COMBATES_TOTALES; n++){
        if(combates[n].jugado && combates[n].perdedor == id && n != 61 && n != 62){
            ronda = nombre_ronda(n);
        }
    }

    // si el grupo ya termino y no quedo entre los dos primeros, quedo eliminado en la fase de grupos
    if(ronda == NULL && total_grupos == MAX_GRUPOS){
        indice = buscar_entrenador(id);
        g = grupo_de_indice(indice);
        if(g >= 0 && grupo_completo(g) && obtener_clasificado(g, 0) != id && obtener_clasificado(g, 1) != id){
            ronda = NOMBRES_GRUPO[g];
        }
    }
    return ronda;
}

// decide el resultado segun los pokemon derrotados: devuelve el id del que derroto mas, o 0 si empataron
int decidir_ganador(int id1, int id2, int d1, int d2){
    int ganador = 0;

    if(d1 > d2){
        ganador = id1;
    } else if(d2 > d1){
        ganador = id2;
    }
    return ganador;
}

// registra el resultado de un combate despues de validarlo. devuelve 0 si todo salio bien o un codigo de error:
//  1 numero de combate invalido | 2 ya tiene resultado | 3 participantes aun no definidos
//  4 los entrenadores no pertenecen a ese enfrentamiento | 5 entrenador eliminado en una fase anterior
//  6 empate en eliminatoria | 7 pokemon derrotados fuera de rango | 8 grupos sin generar | 10 entrenador no registrado
// los entrenadores se pueden dar en cualquier orden: si vienen invertidos se acomodan solos
int registrar_resultado(int numero, int id1, int id2, int der1, int der2){
    int error = 0, invertido = 0, d1, d2, ganador;

    if(!grupos_generados){
        error = 8;
    } else if(numero < 1 || numero > COMBATES_TOTALES){
        error = 1;
    } else if(buscar_entrenador(id1) < 0 || buscar_entrenador(id2) < 0){
        error = 10;
    } else if(combates[numero].jugado){
        error = 2;
    } else if(combates[numero].id1 == 0 || combates[numero].id2 == 0){
        error = 3;
    } else {
        // revisa si los dos entrenadores son los del enfrentamiento (en orden normal o invertido)
        if(id1 == combates[numero].id1 && id2 == combates[numero].id2){
            invertido = 0;
        } else if(id1 == combates[numero].id2 && id2 == combates[numero].id1){
            invertido = 1;
        } else {
            invertido = -1;
        }

        if(invertido < 0){
            // no pertenecen al combate: si ademas ya estaban eliminados (en eliminatorias) se avisa con un error mas claro
            if(numero > COMBATES_GRUPOS && (eliminado_en(id1) != NULL || eliminado_en(id2) != NULL)){
                error = 5;
            } else {
                error = 4;
            }
        } else if(der1 < 0 || der1 > POKEMON_POR_EQUIPO || der2 < 0 || der2 > POKEMON_POR_EQUIPO){
            error = 7;
        } else {
            // acomoda los datos al orden guardado en el combate
            d1 = invertido ? der2 : der1;
            d2 = invertido ? der1 : der2;
            ganador = decidir_ganador(combates[numero].id1, combates[numero].id2, d1, d2);

            if(ganador == 0 && numero > COMBATES_GRUPOS){
                error = 6; // en eliminatorias no existen los empates
            } else {
                combates[numero].der1 = d1;
                combates[numero].der2 = d2;
                combates[numero].jugado = 1;
                combates[numero].ganador = ganador;

                if(ganador == 0){
                    combates[numero].perdedor = 0;
                } else if(ganador == combates[numero].id1){
                    combates[numero].perdedor = combates[numero].id2;
                } else {
                    combates[numero].perdedor = combates[numero].id1;
                }

                historial[total_historial] = numero;
                total_historial++;

                actualizar_torneo();
            }
        }
    }
    return error;
}

// traduce el codigo de error de registrar_resultado a un mensaje para el usuario
const char *texto_error(int codigo){
    const char *mensaje = "Error desconocido.";

    switch(codigo){
        case 1:  mensaje = "El numero de combate debe estar entre 1 y 64."; break;
        case 2:  mensaje = "Ese combate ya tiene un resultado registrado."; break;
        case 3:  mensaje = "Los participantes de ese combate todavia no estan definidos (faltan resultados anteriores)."; break;
        case 4:  mensaje = "Los entrenadores indicados no pertenecen a ese enfrentamiento."; break;
        case 5:  mensaje = "Un entrenador indicado ya fue eliminado en una fase anterior y no puede volver a competir."; break;
        case 6:  mensaje = "En las eliminatorias no puede haber empates: un entrenador debe derrotar a mas pokemon que el otro."; break;
        case 7:  mensaje = "Los pokemon derrotados deben estar entre 0 y 6."; break;
        case 8:  mensaje = "Todavia no se han generado los grupos."; break;
        case 10: mensaje = "Alguno de los identificadores no corresponde a un entrenador registrado."; break;
    }
    return mensaje;
}

// despues de registrar un resultado por teclado, avisa si un grupo termino (y quienes clasificaron) o si hay campeon
void anunciar_avance(int numero){
    int g;

    if(numero <= COMBATES_GRUPOS){
        g = (numero - 1) / COMBATES_POR_GRUPO;
        if(grupo_completo(g)){
            printf("\n%s finalizado. Clasifican: 1%c %s y 2%c %s\n", NOMBRES_GRUPO[g],
                   'A' + g, nombre_de(obtener_clasificado(g, 0)), 'A' + g, nombre_de(obtener_clasificado(g, 1)));
        }
    } else if(numero == 63 || numero == 64){
        if(torneo_terminado()){
            printf("\nEl torneo termino. El campeon es: %s\n", nombre_de(combates[64].ganador));
        }
    }
}

// carga resultados escribiendolos por teclado (se puede repetir las veces que se quiera)
void cargar_resultado_teclado(void){
    int numero, id1, id2, der1, der2, error, otro = 1;

    if(preparar_torneo()){
        while(otro && !fin_de_entrada){
            numero = leer_entero("\nNumero de combate (1-64): ", 1, COMBATES_TOTALES);

            if(!fin_de_entrada){
                if(combates[numero].id1 != 0 && combates[numero].id2 != 0){
                    printf("Combate %d (%s): %s (ID %d) vs %s (ID %d)\n", numero, nombre_ronda(numero),
                           nombre_de(combates[numero].id1), combates[numero].id1,
                           nombre_de(combates[numero].id2), combates[numero].id2);
                }

                id1 = leer_entero("ID del entrenador 1: ", 1, 999999);
                id2 = leer_entero("ID del entrenador 2: ", 1, 999999);
                der1 = leer_entero("Pokemon rivales derrotados por el entrenador 1 (0-6): ", 0, POKEMON_POR_EQUIPO);
                der2 = leer_entero("Pokemon rivales derrotados por el entrenador 2 (0-6): ", 0, POKEMON_POR_EQUIPO);

                if(!fin_de_entrada){
                    error = registrar_resultado(numero, id1, id2, der1, der2);

                    if(error == 0){
                        printf("\nResultado registrado:\n");
                        imprimir_combate(stdout, numero);
                        anunciar_avance(numero);
                    } else {
                        printf("\nNo se pudo registrar: %s\n", texto_error(error));
                    }
                    otro = leer_entero("\nCargar otro resultado? (1 = si, 0 = no): ", 0, 1);
                }
            }
        }
    }
}

// carga resultados desde un archivo de texto. cada linea: numero_combate id1 id2 derrotados1 derrotados2
// las lineas vacias o que empiezan con # se ignoran. permite cargar resultados parciales y no importa el orden de las lineas:
// se repiten pasadas hasta que ya no se pueda aceptar ninguna mas (asi una eliminatoria puede venir antes que sus cruces previos)
void cargar_resultados_archivo(const char *nombre){
    FILE *f;
    char linea[200];
    int datos[MAX_LINEAS_ARCHIVO][5];
    int pendiente[MAX_LINEAS_ARCHIVO];
    int num_linea[MAX_LINEAS_ARCHIVO];
    int codigo[MAX_LINEAS_ARCHIVO];
    int cantidad = 0, linea_actual = 0, mal_formato = 0, excedidas = 0;
    int aceptados = 0, rechazados = 0, progreso = 1, i, e, p, leidos;

    if(preparar_torneo()){
        f = fopen(nombre, "r");

        if(f == NULL){
            printf("No se pudo abrir el archivo '%s'. Revisa el nombre y la carpeta.\n", nombre);
        } else {
            // 1) lee todas las lineas validas del archivo
            while(fgets(linea, sizeof(linea), f) != NULL){
                linea_actual++;
                p = 0;
                while(linea[p] == ' ' || linea[p] == '\t'){
                    p++;
                }

                if(linea[p] != '\0' && linea[p] != '\n' && linea[p] != '\r' && linea[p] != '#'){
                    if(cantidad >= MAX_LINEAS_ARCHIVO){
                        excedidas++;
                    } else {
                        leidos = sscanf(linea, "%d %d %d %d %d", &datos[cantidad][0], &datos[cantidad][1],
                                        &datos[cantidad][2], &datos[cantidad][3], &datos[cantidad][4]);
                        if(leidos == 5){
                            pendiente[cantidad] = 1;
                            num_linea[cantidad] = linea_actual;
                            codigo[cantidad] = 0;
                            cantidad++;
                        } else {
                            mal_formato++;
                            printf("Linea %d: formato invalido (se esperan 5 numeros enteros).\n", linea_actual);
                        }
                    }
                }
            }
            fclose(f);

            // 2) intenta registrar las lineas pendientes, repitiendo mientras alguna se acepte
            while(progreso){
                progreso = 0;
                for(i = 0; i < cantidad; i++){
                    if(pendiente[i]){
                        e = registrar_resultado(datos[i][0], datos[i][1], datos[i][2], datos[i][3], datos[i][4]);
                        // conserva el error mas informativo: no reemplaza un error especifico por "ya registrado" (2) ni "sin definir" (3)
                        if(codigo[i] == 0 || codigo[i] == 3 || (e != 2 && e != 3)){
                            codigo[i] = e;
                        }
                        if(e == 0){
                            pendiente[i] = 0;
                            aceptados++;
                            progreso = 1;
                        }
                    }
                }
            }

            // 3) informa las lineas que no se pudieron registrar
            for(i = 0; i < cantidad; i++){
                if(pendiente[i]){
                    rechazados++;
                    printf("Linea %d (combate %d) rechazada: %s\n", num_linea[i], datos[i][0], texto_error(codigo[i]));
                }
            }

            if(excedidas > 0){
                printf("Se ignoraron %d lineas por superar el maximo de %d.\n", excedidas, MAX_LINEAS_ARCHIVO);
            }
            printf("\nResultados aceptados: %d | rechazados: %d | con formato invalido: %d\n", aceptados, rechazados, mal_formato);
        }
    }
}

// guarda los resultados registrados (en el orden en que se cargaron) en un archivo de texto
// el formato es el mismo que lee cargar_resultados_archivo, asi se puede volver a cargar despues
// devuelve 1 si se guardo bien, 0 si no se pudo crear el archivo
int guardar_historial(const char *nombre){
    FILE *f = fopen(nombre, "w");
    int i, n, ok = 0;

    if(f != NULL){
        fprintf(f, "# numero_combate id_entrenador1 id_entrenador2 pokemon_derrotados_por_1 pokemon_derrotados_por_2\n");
        for(i = 0; i < total_historial; i++){
            n = historial[i];
            fprintf(f, "%d %d %d %d %d\n", n, combates[n].id1, combates[n].id2, combates[n].der1, combates[n].der2);
        }
        fclose(f);
        ok = 1;
    }
    return ok;
}

// exporta la tabla de clasificacion de todos los grupos (y el podio si ya hay) a un archivo de texto
// devuelve 1 si se guardo bien, 0 si no se pudo crear el archivo
int exportar_clasificacion(const char *nombre){
    FILE *f = fopen(nombre, "w");
    int ok = 0;

    if(f != NULL){
        fprintf(f, "TABLA DE CLASIFICACION - TORNEO POKEMON\n");
        imprimir_clasificacion(f);
        if(total_grupos == MAX_GRUPOS){
            imprimir_podio(f);
        }
        fclose(f);
        ok = 1;
    }
    return ok;
}


// ===========================================================================
// IMPRESION DE TABLAS Y LISTAS (reciben stdout para pantalla o un archivo abierto)
// ===========================================================================

// imprime una linea con un combate: participantes y, si ya se jugo, el marcador y el ganador
void imprimir_combate(FILE *s, int n){
    fprintf(s, "  Combate %2d: %-18.18s vs %-18.18s", n, nombre_de(combates[n].id1), nombre_de(combates[n].id2));

    if(combates[n].jugado){
        fprintf(s, "  [%d - %d]  ", combates[n].der1, combates[n].der2);
        if(combates[n].ganador == 0){
            fprintf(s, "Empate\n");
        } else {
            fprintf(s, "Gana %s\n", nombre_de(combates[n].ganador));
        }
    } else {
        fprintf(s, "  (pendiente)\n");
    }
}

// imprime la tabla de posiciones de cada grupo, ya ordenada con los criterios de desempate
void imprimir_clasificacion(FILE *s){
    int g, j, indice, completo;
    char clasif[8];

    for(g = 0; g < total_grupos; g++){
        completo = grupo_completo(g);

        fprintf(s, "\n%s  (combates jugados: %d/%d)\n", NOMBRES_GRUPO[g], jugados_grupo(g), COMBATES_POR_GRUPO);
        fprintf(s, "%-4s %-5s %-20s %4s %3s %3s %3s %7s  %s\n", "Pos", "ID", "Entrenador", "Pts", "PG", "PE", "PP", "PokDer", "Clasif");

        for(j = 0; j < TAM_GRUPO; j++){
            indice = orden_grupo[g][j];

            if(completo && j < 2){
                sprintf(clasif, "%d%c", j + 1, 'A' + g); // ej: 1A, 2A
            } else {
                strcpy(clasif, "-");
            }

            fprintf(s, "%-4d %-5d %-20.20s %4d %3d %3d %3d %7d  %s\n", j + 1, entrenadores[indice].Identificador,
                    entrenadores[indice].Nombre, estadisticas[indice].puntos, estadisticas[indice].victorias,
                    estadisticas[indice].empates, estadisticas[indice].derrotas, estadisticas[indice].derrotados, clasif);
        }
    }
    fprintf(s, "\nPts = puntos | PG = ganados | PE = empates | PP = perdidos | PokDer = pokemon rivales derrotados\n");
    fprintf(s, "Desempate: 1) mas puntos  2) mas victorias  3) mas pokemon rivales derrotados\n");
}

// imprime el podio oficial (campeon, subcampeon, tercer y cuarto lugar) o cuanto falta para tenerlo
void imprimir_podio(FILE *s){
    int n, jugados = 0;

    if(torneo_terminado()){
        fprintf(s, "\n===== PODIO OFICIAL =====\n");
        fprintf(s, "Campeon:      %s\n", nombre_de(combates[64].ganador));
        fprintf(s, "Subcampeon:   %s\n", nombre_de(combates[64].perdedor));
        fprintf(s, "Tercer lugar: %s\n", nombre_de(combates[63].ganador));
        fprintf(s, "Cuarto lugar: %s\n", nombre_de(combates[63].perdedor));
    } else {
        for(n = 49; n <= COMBATES_TOTALES; n++){
            if(combates[n].jugado){
                jugados++;
            }
        }
        fprintf(s, "\nEl torneo aun no termina (combates eliminatorios jugados: %d/%d).\n", jugados, COMBATES_TOTALES - COMBATES_GRUPOS);
    }
}

// imprime el calendario de combates. fase 1 = fase de grupos | fase 2 = eliminatorias
void imprimir_enfrentamientos(int fase){
    int g, k, n;

    if(fase == 1){
        for(g = 0; g < total_grupos; g++){
            printf("\n%s\n", NOMBRES_GRUPO[g]);
            for(k = 1; k <= COMBATES_POR_GRUPO; k++){
                imprimir_combate(stdout, g * COMBATES_POR_GRUPO + k);
            }
        }
    } else {
        if(total_grupos != MAX_GRUPOS){
            printf("\nLa fase eliminatoria solo se habilita con %d entrenadores (%d grupos). Hay %d registrados.\n",
                   MAX_ENTRENADORES, MAX_GRUPOS, total_entrenadores);
        } else {
            for(n = 49; n <= COMBATES_TOTALES; n++){
                if(n == 49){
                    printf("\nOCTAVOS DE FINAL\n");
                } else if(n == 57){
                    printf("\nCUARTOS DE FINAL\n");
                } else if(n == 61){
                    printf("\nSEMIFINALES\n");
                } else if(n == 63){
                    printf("\nTERCER LUGAR\n");
                } else if(n == 64){
                    printf("\nFINAL\n");
                }
                imprimir_combate(stdout, n);
            }
        }
    }
}

// imprime los resultados registrados en el orden en que se cargaron
void imprimir_historial(FILE *s){
    int i;

    if(total_historial == 0){
        fprintf(s, "\nTodavia no se ha registrado ningun resultado.\n");
    } else {
        fprintf(s, "\nHISTORIAL DE RESULTADOS (%d registrados)\n", total_historial);
        for(i = 0; i < total_historial; i++){
            fprintf(s, "%3d.", i + 1);
            imprimir_combate(s, historial[i]);
        }
    }
}


// ===========================================================================
// 5. MENU PRINCIPAL Y OPCIONES
// ===========================================================================

// dibuja el menu con las 12 opciones
void mostrar_menu(void){
    printf("\n========================================\n");
    printf("          TORNEO POKEMON - MENU\n");
    printf("========================================\n");
    printf("  1. Consultar Pokedex\n");
    printf("  2. Registrar entrenador\n");
    printf("  3. Crear equipo\n");
    printf("  4. Consultar entrenadores\n");
    printf("  5. Consultar equipos\n");
    printf("  6. Cargar resultados\n");
    printf("  7. Consultar clasificacion\n");
    printf("  8. Consultar enfrentamientos\n");
    printf("  9. Historial\n");
    printf(" 10. Resultados globales\n");
    printf(" 11. Mostrar campeon\n");
    printf(" 12. Salir\n");
    printf("========================================\n");
}

// opcion 6: elige si los resultados se cargan por teclado o desde un archivo
void menu_cargar_resultados(void){
    int opcion;
    char archivo[100];

    printf("\n--- CARGAR RESULTADOS ---\n");
    printf("  1. Por teclado\n");
    printf("  2. Desde un archivo de texto\n");
    printf("  0. Volver\n");
    opcion = leer_entero("Elige una opcion: ", 0, 2);

    if(!fin_de_entrada){
        if(opcion == 1){
            cargar_resultado_teclado();
        } else if(opcion == 2){
            printf("Formato de cada linea: numero_combate id1 id2 derrotados1 derrotados2\n");
            leer_texto("Nombre del archivo (ej: resultados.txt): ", archivo, sizeof(archivo));
            if(!fin_de_entrada){
                cargar_resultados_archivo(archivo);
            }
        }
    }
}

// opcion 7: muestra las tablas de cada grupo y ofrece exportarlas a un archivo
void consultar_clasificacion(void){
    int exportar;

    if(preparar_torneo()){
        imprimir_clasificacion(stdout);
        exportar = leer_entero("\nExportar la tabla a '" ARCHIVO_CLASIFICACION "'? (1 = si, 0 = no): ", 0, 1);

        if(!fin_de_entrada && exportar == 1){
            if(exportar_clasificacion(ARCHIVO_CLASIFICACION)){
                printf("Tabla exportada en '%s'.\n", ARCHIVO_CLASIFICACION);
            } else {
                printf("No se pudo crear el archivo '%s'.\n", ARCHIVO_CLASIFICACION);
            }
        }
    }
}

// opcion 8: muestra el calendario de la fase de grupos o de las eliminatorias
void consultar_enfrentamientos(void){
    int fase;

    if(preparar_torneo()){
        printf("\n  1. Fase de grupos\n");
        printf("  2. Fase eliminatoria\n");
        fase = leer_entero("Elige la fase: ", 1, 2);

        if(!fin_de_entrada){
            imprimir_enfrentamientos(fase);
        }
    }
}

// opcion 9: muestra el historial y ofrece guardarlo en un archivo
void consultar_historial(void){
    int guardar;

    imprimir_historial(stdout);

    if(total_historial > 0){
        guardar = leer_entero("\nGuardar el historial en '" ARCHIVO_HISTORIAL "'? (1 = si, 0 = no): ", 0, 1);

        if(!fin_de_entrada && guardar == 1){
            if(guardar_historial(ARCHIVO_HISTORIAL)){
                printf("Historial guardado en '%s'.\n", ARCHIVO_HISTORIAL);
            } else {
                printf("No se pudo crear el archivo '%s'.\n", ARCHIVO_HISTORIAL);
            }
        }
    }
}

// opcion 10: resumen general (avance, tabla general de todos los entrenadores y resultados de eliminatorias)
void mostrar_resultados_globales(void){
    int i, j, k, temporal, mover, jugados_grupos = 0, jugados_elim = 0, n;
    int orden[MAX_ENTRENADORES];

    if(preparar_torneo()){
        for(n = 1; n <= COMBATES_GRUPOS; n++){
            if(combates[n].jugado){
                jugados_grupos++;
            }
        }
        for(n = 49; n <= COMBATES_TOTALES; n++){
            if(combates[n].jugado){
                jugados_elim++;
            }
        }

        printf("\n===== RESULTADOS GLOBALES =====\n");
        printf("Entrenadores: %d | Grupos: %d\n", total_entrenadores, total_grupos);
        printf("Combates de grupos jugados: %d/%d\n", jugados_grupos, total_grupos * COMBATES_POR_GRUPO);
        if(total_grupos == MAX_GRUPOS){
            printf("Combates eliminatorios jugados: %d/%d\n", jugados_elim, COMBATES_TOTALES - COMBATES_GRUPOS);
        }

        // ordena a todos los entrenadores juntos con los mismos criterios de desempate (insercion)
        for(i = 0; i < total_entrenadores; i++){
            orden[i] = i;
        }
        for(i = 1; i < total_entrenadores; i++){
            temporal = orden[i];
            j = i - 1;
            mover = 1;
            while(j >= 0 && mover){
                if(es_mejor(temporal, orden[j])){
                    orden[j + 1] = orden[j];
                    j--;
                } else {
                    mover = 0;
                }
            }
            orden[j + 1] = temporal;
        }

        printf("\nTABLA GENERAL (puntos de la fase de grupos)\n");
        printf("%-4s %-5s %-20s %-8s %4s %3s %3s %3s %7s\n", "Pos", "ID", "Entrenador", "Grupo", "Pts", "PG", "PE", "PP", "PokDer");
        for(k = 0; k < total_entrenadores; k++){
            i = orden[k];
            printf("%-4d %-5d %-20.20s %-8s %4d %3d %3d %3d %7d\n", k + 1, entrenadores[i].Identificador, entrenadores[i].Nombre,
                   NOMBRES_GRUPO[grupo_de_indice(i)], estadisticas[i].puntos, estadisticas[i].victorias,
                   estadisticas[i].empates, estadisticas[i].derrotas, estadisticas[i].derrotados);
        }

        if(total_grupos == MAX_GRUPOS){
            imprimir_enfrentamientos(2);
            imprimir_podio(stdout);
        }
    }
}

// opcion 11: muestra el podio oficial (solo cuando la final y el tercer puesto ya se jugaron)
void mostrar_campeon(void){
    if(preparar_torneo()){
        if(total_grupos != MAX_GRUPOS){
            printf("\nLa fase eliminatoria solo se habilita con %d entrenadores (%d grupos). Hay %d registrados.\n",
                   MAX_ENTRENADORES, MAX_GRUPOS, total_entrenadores);
        } else {
            imprimir_podio(stdout);
        }
    }
}

// ciclo principal del programa: muestra el menu y ejecuta la opcion elegida hasta que se elija 12 (salir)
void menu_principal(void){
    int opcion = 0;

    while(opcion != 12 && !fin_de_entrada){
        mostrar_menu();
        opcion = leer_entero("Elige una opcion (1-12): ", 1, 12);

        if(!fin_de_entrada){
            switch(opcion){
                case 1:  consultar_pokedex();             break;
                case 2:  registrar_entrenador();          break;
                case 3:  crear_equipo();                  break;
                case 4:  consultar_entrenadores();        break;
                case 5:  consultar_equipos();             break;
                case 6:  menu_cargar_resultados();        break;
                case 7:  consultar_clasificacion();       break;
                case 8:  consultar_enfrentamientos();     break;
                case 9:  consultar_historial();           break;
                case 10: mostrar_resultados_globales();   break;
                case 11: mostrar_campeon();               break;
                case 12: printf("\nHasta luego!\n");      break;
            }
        }
    }
}


// ===========================================================================
// VERSIONES PROVISIONALES DE LAS OPCIONES 1 A 5 (de otros integrantes)
// ===========================================================================
// sirven para que el menu compile y se pueda probar el torneo ya mismo.
// cuando existan las funciones reales de tus companeros, quita el #define USAR_FUNCIONES_PROVISIONALES de arriba
// (y ajusta los nombres si sus funciones se llaman distinto o reciben parametros)
#ifdef USAR_FUNCIONES_PROVISIONALES

// llena los datos iniciales de un entrenador nuevo
void iniciar_entrenador(int indice, int id, const char *nombre){
    entrenadores[indice].Identificador = id;
    strncpy(entrenadores[indice].Nombre, nombre, 49);
    entrenadores[indice].Nombre[49] = '\0';
    strcpy(entrenadores[indice].Equipo_Pokemon, "(sin equipo)");
    entrenadores[indice].Cantidad_de_victorias = 0;
    entrenadores[indice].Cantidad_de_empates = 0;
    entrenadores[indice].Cantidad_de_derrotas = 0;
    entrenadores[indice].Puntuacion = 0;
}

// guarda la lista de entrenadores (un entrenador por linea: id;nombre) para no tener que registrarlos otra vez
void guardar_entrenadores_archivo(void){
    FILE *f = fopen(ARCHIVO_ENTRENADORES, "w");
    int i;

    if(f != NULL){
        for(i = 0; i < total_entrenadores; i++){
            fprintf(f, "%d;%s\n", entrenadores[i].Identificador, entrenadores[i].Nombre);
        }
        fclose(f);
    }
}

// lee entrenadores.txt al iniciar el programa (si existe)
void cargar_entrenadores_archivo(void){
    FILE *f = fopen(ARCHIVO_ENTRENADORES, "r");
    char linea[200];
    char *separador;
    int id;

    if(f != NULL){
        while(fgets(linea, sizeof(linea), f) != NULL && total_entrenadores < MAX_ENTRENADORES){
            limpiar_linea(linea);
            separador = strchr(linea, ';');

            if(separador != NULL){
                *separador = '\0';
                id = atoi(linea);
                if(id > 0 && buscar_entrenador(id) < 0 && separador[1] != '\0'){
                    iniciar_entrenador(total_entrenadores, id, separador + 1);
                    total_entrenadores++;
                }
            }
        }
        fclose(f);
    }
}

void consultar_pokedex(void){
    printf("\n[Pendiente] La consulta de Pokedex la implementa otro integrante del equipo.\n");
}

// registro minimo: pide el nombre y asigna un ID correlativo
void registrar_entrenador(void){
    char nombre[50];
    int i, id = 0;

    if(total_historial > 0){
        printf("\nNo se pueden registrar entrenadores porque ya hay resultados cargados.\n");
    } else if(total_entrenadores >= MAX_ENTRENADORES){
        printf("\nSe alcanzo el maximo de %d entrenadores.\n", MAX_ENTRENADORES);
    } else {
        leer_texto("\nNombre del entrenador: ", nombre, sizeof(nombre));

        if(!fin_de_entrada){
            // el ; se usa como separador en el archivo, asi que se cambia por un espacio
            for(i = 0; nombre[i] != '\0'; i++){
                if(nombre[i] == ';'){
                    nombre[i] = ' ';
                }
            }

            // el nuevo ID es el mayor existente + 1
            for(i = 0; i < total_entrenadores; i++){
                if(entrenadores[i].Identificador > id){
                    id = entrenadores[i].Identificador;
                }
            }
            id++;

            iniciar_entrenador(total_entrenadores, id, nombre);
            total_entrenadores++;
            guardar_entrenadores_archivo();
            printf("Entrenador registrado con ID %d (%d/%d).\n", id, total_entrenadores, MAX_ENTRENADORES);
        }
    }
}

void crear_equipo(void){
    printf("\n[Pendiente] La creacion de equipos la implementa otro integrante del equipo.\n");
}

void consultar_entrenadores(void){
    int i;

    if(total_entrenadores == 0){
        printf("\nNo hay entrenadores registrados.\n");
    } else {
        printf("\n%-5s %-20s %-14s %3s %3s %3s %4s\n", "ID", "Entrenador", "Equipo", "PG", "PE", "PP", "Pts");
        for(i = 0; i < total_entrenadores; i++){
            printf("%-5d %-20.20s %-14.14s %3d %3d %3d %4d\n", entrenadores[i].Identificador, entrenadores[i].Nombre,
                   entrenadores[i].Equipo_Pokemon, entrenadores[i].Cantidad_de_victorias, entrenadores[i].Cantidad_de_empates,
                   entrenadores[i].Cantidad_de_derrotas, entrenadores[i].Puntuacion);
        }
    }
}

void consultar_equipos(void){
    printf("\n[Pendiente] La consulta de equipos la implementa otro integrante del equipo.\n");
}

#endif


// ===========================================================================
// PROGRAMA PRINCIPAL
// ===========================================================================
// nota: el main de prueba del combate 1v1 que existia antes se renombro a prueba_combate_1v1()
// (su contenido no cambio) porque en C solo puede haber un main

int main(){
    #ifdef USAR_FUNCIONES_PROVISIONALES
    cargar_entrenadores_archivo(); // recupera los entrenadores registrados en ejecuciones anteriores
    #endif

    menu_principal();

    return 0;
}

// ============================================================================
// ======================== FIN DE MI PARTE (PUNTOS 4 Y 5) ====================
// ============================================================================
