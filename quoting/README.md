# Zsh Quoting Performance Tests

- `short.zsh` – test of quoting, `(q)`-flag vs. `printf '%q '`,  short input string
  * result: 35 ms vs. 26.6 sec
- `Q_short.zsh` – test of dequoting, `(Q)`-flag vs. `eval "str=$quoted"`, short input string
  * result: 25 ms vs. 77 ms

Bash 4.4 performance of `printf -v OUTPUT '%q ' "$INPUT"`: 227 ms
