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

- **R** → Rey  
- **D** → Dama  
- **T** → Torre  
- **A** → Alfil  
- **C** → Caballo  
- **(sin letra)** → Peón  

> Los peones no llevan letra; su movimiento se indica solo con la casilla de destino.

---

### Cómo se escriben los movimientos
- **Piezas (excepto peones):**
  - Letra de la pieza + casilla de destino.  
  - Ejemplos:  
    - `Cg5` → un caballo va a g5  
    - `Dd4` → la dama va a d4  

- **Peones:**
  - Solo casilla de destino.  
  - Ejemplo: `e5` → un peón avanza a e5  

---

### Ventajas
- Registro claro y universal de movimientos.  
- Fácil de leer, escribir y analizar.  
- Obligatoria en torneos oficiales.





SISTEMA AMIGABLE CON EL USUSARIO Y OFRECE UNA METODOLOGIA ESPECIALIZZADA Y DIRECTA.


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


Esto es lo que validamos en el análisis sintáctico: verificamos que cada movimiento pertenezca al lenguaje formal $L$ definido anteriormente. El parser recorre secuencialmente los tokens y valida que la estructura del movimiento corresponda exactamente a una de las producciones gramaticales especificadas.







##  Cómo compilar

```bash
make
./chess
