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

int main (){
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