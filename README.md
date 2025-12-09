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


## Ánalsis Lexico

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








##  Cómo compilar

```bash
make
./chess
