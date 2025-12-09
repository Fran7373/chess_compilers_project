# Proyecto Chess Compiler

El objetivo del proyecto es construir una aplicación con el lenguaje C/C++, el cual sea un analizador léxico, sintáctico y semántico que valide los movimientos de la notación algebraica del ajedrez y determine si el movimiento es:

- Legal o ilegal.  
- Es jaque o jaque mate.  
- Contiene ambigüedades.  
- Respeta las reglas oficiales del ajedrez.  
- Produce un nuevo estado de tablero válido.


## Escritura SAN

Que es la escritura SAN?

## Ánalsis Lexico

Para analisar Lexicamente la notación algebraica definimos los siguientes tokens

```C
TK_UNKNOWN
TK_PIECE
TK_FILE
 TK_RANK
TK_CAPTURE;
TK_PROMOTE
TK_PROMOTE_PIECE
TK_CHECK
TK_MATE
TK_CASTLE_SHORT
TK_CASTLE_LONG
TK_END
```








##  Cómo compilar

```bash
make
./chess
