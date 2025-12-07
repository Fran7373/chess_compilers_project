# Compilador de Notación Algebraica de Ajedrez (SAN)
Proyecto académico para la materia **Compiladores**, cuyo objetivo es implementar
las tres fases fundamentales de un compilador:

1. **Análisis Léxico**
2. **Análisis Sintáctico**
3. **Análisis Semántico**

El lenguaje fuente es la **notación algebraica estándar de ajedrez (SAN)**, y el
programa interpreta jugadas y las aplica a un tablero real, validando que 
sean legales de acuerdo a las reglas oficiales de la FIDE.

---

## 📌 1. Análisis Léxico (Lexer)

El lexer toma una cadena SAN como:

```
exd6
Qe2
O-O
R1a3
e8=Q
```

y produce una secuencia de tokens, por ejemplo:

```
TK_FILE     'e'
TK_CAPTURE  'x'
TK_FILE     'd'
TK_RANK     '6'
TK_END      ''
```

El lexer:

- reconoce símbolos: `x`, `+`, `#`, `=`
- detecta piezas: `K, Q, R, B, N`
- reconoce filas (`1–8`) y columnas (`a–h`)
- identifica enroques `O-O` y `O-O-O`
- genera `TK_END` al finalizar

---

## 📌 2. Análisis Sintáctico (Parser)

El parser toma la lista de tokens y construye un AST (`MoveAST`) con campos como:

```
piece: 'P'
src_file: 'e'
dest_file: 'd'
dest_rank: '6'
is_capture: 1
promotion: '-'
is_castle_short: 0
```

El parser valida:

- forma correcta del movimiento
- estructura de enroques
- estructura de promoción
- ambigüedades como `Nbd2` o `R1e2`

Si el movimiento no pertenece al lenguaje SAN, se reporta **error sintáctico**.

---

## 📌 3. Análisis Semántico

La fase semántica valida si el movimiento **tiene sentido en el tablero real**:

### ✔ Movimientos legales implementados
- Movimiento correcto de todas las piezas
- Capturas
- Peones (avance simple, doble, captura normal, promoción, **captura al paso**)  
- Enroque corto y largo con reglas completas:
  - ni rey ni torre deben haber movido
  - el rey no puede atravesar casillas atacadas
  - no puede terminar en jaque
- Prohibición de capturar al rey enemigo
- Movimiento inválido si deja al rey propio en jaque
- Desambiguación de movimientos SAN

### ✔ Si el movimiento es ilegal
El motor imprime un **error semántico** explicando el motivo, por ejemplo:

```
Error semántico: No se encontró ningún peón que pueda jugar exd6
Error semántico: Movimiento ilegal: el rey quedaría en jaque tras Qe2
Error semántico: Movimiento ilegal: el peón que llega a la última fila debe promocionar
```

---

## 📌 Ejecución

Compilar:

```
make
```

Ejecutar:

```
./build/chess_compiler
```

Ejemplo de sesión:

```
Ingrese un movimiento SAN: e4
Tokens detectados...
Parsed MoveAST...
Movimiento aplicado correctamente.
```

---

## 📌 Tablero

El programa imprime el tablero en formato:

```
8  r n b q k b n r
7  p p p . p p p p
...
1  R N B Q K B N R
   a b c d e f g h
```

---

## 📌 Créditos
Proyecto desarrollado para comprender profundamente las fases de un compilador aplicadas a un lenguaje formal (SAN), integrando diseño de AST, validación semántica y ejecución sobre un modelo de dominio no trivial: ajedrez.
