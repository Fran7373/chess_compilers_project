# Proyecto Chess Compiler.


El objetivo del proyecto es construir una aplicación con el lenguaje C/C++, el cual sea un analizador léxico, sintáctico y semántico que valide los movimientos de la notación algebraica del ajedrez y determine si el movimiento es:

- Legal o ilegal.  
- Es jaque o jaque mate.  
- Contiene ambigüedades.  
- Respeta las reglas oficiales del ajedrez.  
- Produce un nuevo estado de tablero válido.

# Conceptos clave para el razonamiento de Chess Compiler
# SAN  (Standard Algebraic Notation) o Notacion Algebraica Estandar en español

La **notación algebraica** es el sistema estándar para registrar partidas de ajedrez.  
Asigna un nombre único a cada casilla del tablero y permite describir cada movimiento de forma precisa y universal.  
Es la notación oficial utilizada en torneos.

---

### Estructura del tablero
- El tablero tiene **8 columnas (a–h)** y **8 filas (1–8)**.  
- Cada casilla se identifica con una combinación letra+número:  
  - Ejemplos: `a1`, `e4`, `h8`.

---

### Identificación de piezas
En español, las piezas se representan con letras mayúsculas:

- **K** → Rey  
- **Q** → Dama  
- **R** → Torre  
- **B** → Alfil  
- **N** → Caballo  
- **(sin letra)** → Peón  

> Los peones no llevan letra; su movimiento se indica solo con la casilla de destino.

---

### Cómo se escriben los movimientos
- **Piezas (excepto peones):**
  - Letra de la pieza + casilla de destino.  
  - Ejemplos:  
    - `kg5` → un caballo va a g5  
    - `Qd4` → la dama va a d4  

- **Peones:**
  - Solo casilla de destino.  
  - Ejemplo: `e5` → un peón avanza a e5  

---

### Ventajas
- Registro claro y universal de movimientos.  
- Fácil de leer, escribir y analizar.  
- Obligatoria en torneos oficiales.





SISTEMA AMIGABLE CON EL USUSARIO Y OFRECE UNA METODOLOGIA ESPECIALIZZADA Y DIRECTA.




## ¿Qué es PGN en ajedrez?

**PGN** (*Portable Game Notation*) es el formato estándar para almacenar y compartir partidas de ajedrez en archivos de texto simples.  
Fue creado por Steven J. Edwards y es compatible con prácticamente todos los programas y bases de datos de ajedrez.

A diferencia del formato **FEN**, que describe una única posición, **PGN registra la partida completa**, incluyendo todos los movimientos y metadatos relevantes.

---

### ¿Qué contiene un archivo PGN?

Un archivo PGN se divide en **dos secciones**:

---

#### **Sección de etiquetas (Metadata)**  
Contiene información adicional sobre la partida en formato `[Clave "Valor"]`.  
Ejemplos comunes:

- **Evento** → Nombre del torneo  
- **Sitio** → Lugar donde se jugó  
- **Fecha** → Fecha del encuentro  
- **Ronda** → Ronda del torneo  
- **White** → Jugador de piezas blancas  
- **Black** → Jugador de piezas negras  
- **Result** → Resultado (`1-0`, `0-1`, `1/2-1/2`)

---

#### **Sección de movimientos**
Lista de movimientos escritos en **SAN (Standard Algebraic Notation)**.  
Ejemplo:

e4 e5

Nf3 Nc6

Bb5 a6


### Ventajas del formato PGN

- Fácil de leer por humanos y computadoras.  
- Permite cargar partidas completas en segundos.  
- Útil para análisis, estudios, comentarios y bases de datos.  
- Estándar ampliamente aceptado en herramientas y motores de ajedrez.  



## Ánalsis lexico

Para analisar Lexicamente la notación algebraica definimos los siguientes tokens

```C
TK_UNKNOWN // Cualquier cosa que no reconozca el analizador lexico
TK_PIECE //Las letras en mayúscula de las piezas K (Rey), Q (Dama), R (Torre), B (Alfil), N (Caballo) y P (Peón)
         //Si bien cuando movemos el peón no escribimos la letra P, igual usamos el token como idientficador
TK_FILE  //Columna del tablero que pueden ser de las letras a-h
TK_RANK  //Fila del tablero que pueden ser los números entre 1-8
TK_CAPTURE; // Token de captura x O x
TK_PROMOTE //Simbolo =
TK_PROMOTE_PIECE // Pieza que va despues del =
TK_CHECK // Simbolo +
TK_MATE // Simbolo #
TK_CASTLE_SHORT // Enroque corto:0-0 o O-O
TK_CASTLE_LONG // Enroque largo: 0-0 o O-O-O
TK_END //Siempre se termina en este token
```

El analisis lexico es sencillo, ya que la notación SAN es muy especifica y limitada dentro del ajedrez. Sin embargo tenemos que asegurarnos de buscar el match más largo para la deteción de enroques, ya que de lo contrario la cadena 0-0-0 podría ser idientificada como `TK_CASTLE_SHORT:"0-0"`,`TK_UNKNOWN:"-"` y `TK_UNKNOWN:"0"`, cuando la tokenización correcta es `TK_CASTLE_LONG :"0-0-0"`.



## Ánalisis sintactico,formemos una gramatica para validar la sintaxis.




## Definición del Lenguaje

Sea $L$ el lenguaje de movimientos de ajedrez válidos en notación algebraica estándar (SAN):

$$L = L_{\text{enroque}} \cup L_{\text{movimiento}}$$

### Alfabeto

$$\Sigma = \{K, Q, R, B, N, a, b, c, d, e, f, g, h, 1, 2, 3, 4, 5, 6, 7, 8, x, =, +, \\#\ , O, 0, -\}$$

Donde:
- $P = \{K, Q, R, B, N\}$ es el conjunto de piezas
- $F = \{a, b, c, d, e, f, g, h\}$ es el conjunto de columnas (files)
- $R = \{1, 2, 3, 4, 5, 6, 7, 8\}$ es el conjunto de filas (ranks)
- $P_p = \{Q, R, B, N\}$ es el conjunto de piezas de promoción

### Enroques

$$L_{\text{enroque}} = \{O\text{-}O\text{-}O, 0\text{-}0\text{-}0, O\text{-}O, 0\text{-}0\} \times A$$

donde $A = \{\epsilon, +, \\#\}$ son anotaciones opcionales.

### Movimientos Normales

$$L_{\text{movimiento}} = L_{\text{pieza}} \times D \times C \times L_{\text{destino}} \times M \times A$$

#### Componentes

**Pieza (opcional):**

$$L_{\text{pieza}} = P \cup \{\epsilon\}$$

**Desambiguación:**

$$D = \{\epsilon\} \cup F \cup R \cup (F \times R)$$

**Captura:**

$$C = \{\epsilon, x\}$$

**Destino (obligatorio):**

$$L_{\text{destino}} = F \times R$$

**Promoción:**

$$M = \{\epsilon\} \cup (\{=\} \times P_p)$$

**Anotación:**

$A = \{\epsilon, +, \\#\}$

### Gramática BNF

```bnf
<move> ::= <castle> | <normal-move>

<castle> ::= ("O-O-O" | "0-0-0" | "O-O" | "0-0") <annotation>

<normal-move> ::= <piece> <disambiguation> <capture> <destination> <promotion> <annotation>

<piece> ::= "K" | "Q" | "R" | "B" | "N" | ε

<disambiguation> ::= ε | <file> | <rank> | <file><rank>

<capture> ::= ε | "x"

<destination> ::= <file><rank>

<promotion> ::= ε | "=" <promote-piece>

<promote-piece> ::= "Q" | "R" | "B" | "N"

<annotation> ::= ε | "+" | "#"

<file> ::= "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h"

<rank> ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8"
```

### Ejemplos

| Movimiento | Notación Matemática |
|------------|---------------------|
| `e4` | $\epsilon \times \epsilon \times \epsilon \times (e, 4) \times \epsilon \times \epsilon$ |
| `Nf3` | $N \times \epsilon \times \epsilon \times (f, 3) \times \epsilon \times \epsilon$ |
| `Qh4xe1` | $Q \times (h, 4) \times x \times (e, 1) \times \epsilon \times \epsilon$ |
| `e8=Q#` | $\epsilon \times \epsilon \times \epsilon \times (e, 8) \times (=Q) \times \\#$ |
| `O-O-O` | $\in L_{\text{enroque}}$ |
| `Raxb1` | $R \times a \times \epsilon \times x \times (b, 1) \times \epsilon \times \epsilon$ |
| `N5xd4` | $N \times 5 \times x \times (d, 4) \times \epsilon \times \epsilon$ |

### Cardinalidad

El número máximo de movimientos válidos representables:

$$|L| = |L_{\text{enroque}}| + |L_{\text{movimiento}}|$$

**Enroques:**

$$|L_{\text{enroque}}| = 4 \times 3 = 12$$

**Movimientos normales:**

$$|L_{\text{movimiento}}| \leq 6 \times 129 \times 2 \times 64 \times 5 \times 3 = 2{,}985{,}120$$


Esto es lo que validamos en el análisis sintáctico: verificamos que cada movimiento pertenezca al lenguaje formal $L$ definido anteriormente. El parser recorre secuencialmente los tokens y valida que la estructura del movimiento corresponda exactamente a una de las producciones gramaticales especificadas. Sin embargo no todas las expreciones sintacticamente validas lo son semanticamente. Por ejemplo $Qh4xe1=Q\\#\$ es sintacticamente valido, pero imposible ya que una reina no puede "promocionarse". Está validación la hará el ánalizador semantico.


## Análisis Semántico

Una vez que el lexer y el parser han transformado cada jugada escrita en notación algebraica estándar (SAN) en una estructura sintáctica bien formada (MoveAST), es necesario verificar si dicha jugada tiene sentido dentro de las reglas reales del ajedrez.

Este proceso corresponde al análisis semántico, la etapa del compilador encargada de comprobar que una construcción sintácticamente válida también sea significativa y legal según el modelo del dominio, en este caso, las Leyes de Ajedrez de la FIDE.

Los archivos que contienen el código que realiza este análisis:

`semant.h` 

- Define las estructuras lógicas que usa la semántica:
    - `Color`
    - `PieceType`
    - `Piece`
    - `Board`
    - `PositionStatus`

- Declara las funciones:
    - `board_init_start`: inicializa el tablero
    - `board_apply_move`: realiza el movimiento de las fichas
    - `board_evaluate_status`: valida si el estado del juego está en jaque, jaque mate o tablas
    - `file_to_index`: convierte el número de una posición en un índice
    - `rank_to_index`: convierte la letra del columna de de una posición en un índice

`semant.c` 

Contine la implementación del análisis semántico y las reglas de juego oficial del ajedrez (FIDE). 

- Utilidades
    - `file_to_index`: convierte el número de una posición en un índice
    - `rank_to_index`: convierte la letra del columna de de una posición en un índice
    - `set_piece`: mueve una pieza a una posición indicada
    - `board_init_start`: inicializa el tablero en la posición estándar

- Validación de ataques
    - `is_square_attacked`: valida si una casilla está siendo a tacada por un bando.
    - `is_king_in_check`: valida si el rey está en jaque

- Reglas de movimiento: dentro de estas hay reglas comunes, como no comer fichas del mismo bando.
    - `can_knight_move`: valida el movimiento del caballo.
    - `can_pawn_move`: valida movimiento del peón
        - Avance una o dos pasos
        - Captura en diagonal
        - Captura al paso
        - Promoción al llegar a la última fila
    - `can_bishop_move`: valida el movimiento del alfíl
        - Avance en diagonal
        - Que no se encuentren piezas obstruyendo el camino
    - `can_rook_move`: valida el movimiento de la torre
        - Avance en horizontal y vertical
        - Que no se encuentren piezas obstruyendo el camino
    - `can_queen_move`: valida el movimiento de la dama
        - Avance en diagonal, horizontal y vertical
        - Que no se encuentren piezas obstruyendo el camino
    - `can_king_move`: valida el movimiento del rey

- Búsqueda de la pieza origen
    - Para cada tipo `find_x_source`:
        - `find_knight_source`
        - `find_pawn_source`
        - `find_bishop_source`
        - `find_rook_source`
        - `find_queen_source`
        1. Aplica filtro de ambigüedad
        2. Valida si el movimiento geométricamente es legal
        3. Realiza el movimiento en una copia para validar que el rey propio no quede en jaque

- Derechos de enroque
    - `update_castling_rights_on_move`: valida que la torre y el rey no se muevan antes del enroque para que el movimiento sea legal
    - `update_castling_rights_on_capture`: valida si la torre es capturada para quitar los derechos del enroque de ese lado.
    - `update_castling_rights_on_capture`: 
        - Valida que la torre y el rey estén en la posición correcta
        - Valida que no hayan obstaculos en las casillas intermedias
        - Valida que el rey no esté en jaque, ni antes del movimiento, en las casillas en las que se desplaza, ni en la posición final
        - Realiza le movimiento si este es legal

- Evaluación global de la posición
    - `has_any_legal_move`: valida que al menos haya un movimiento legal de manera que se valide o no si el rey queda ahogado
    - `board_evaluate_status`: indica si hay jaque, jaque mate, ahogado o en juego normal.

- Análisis del movimiento:
    - `board_apply_move`: analiza semanticamente y realiza el movimiento.
        - Toma:
            - Un Board (tablero)
            - Un MoveAST ya parseado (nodo)
            - side_to_move (color)
            - Un buffer error_msg
        1. Valida el enroque
        2. Determina el tipo de pieza
        3. Convierte destino SAN en índices
        4. Valida las reglas de la promoción del peón
        5. Valida si hay ambigüedades
        6. Crea un tablero temporal (simulación)
        7. Valida si hay captura al paso
        8. valida que no se capture al rey enemigo
        9. Actualiza los derechos del enroque
        10. Realiza la simulación del movimiento
        11. Actualiza el derecho de captura al paso
        12. Valida que el rey propio no quede en jaque
        13. Valida que la notación de jaque y jaque mate sean coherentes con el estado del tablero
        14. Si todo es legal, realiza el movimiento de la pieza




##  Cómo compilar

Para una correcta visualización de caracteres en **Windows**, ejecute:

    ```powershell
    $OutputEncoding = [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding```

Para compilar el proyecto:

    gcc -o chess main.c interactivo.c pgn.c lexer.c parser.c semant.c -Wall

Para ejecutar el programa:

    ./chess
