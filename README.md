# Trabalho Prático 1 — Programação Paralela com Threads

Sistemas Operacionais — UFAM

Trabalho de programação paralela com **threads (pthreads)** em C. São duas questões:

1. **Produto escalar** entre dois vetores — versão sequencial vs. paralela.
2. **Multiplicação de matrizes** (a aplicação escolhida) — versão sequencial vs. paralela, com modo interativo.

Em ambas medimos o tempo de execução e calculamos a **Aceleração / Speedup**:

> **Sp = Ts / Tp**, onde `Ts` = tempo da versão sequencial e `Tp` = tempo da versão paralela com `p` threads.

---

## 📁 Estrutura do projeto

| Arquivo | O que é |
|---|---|
| `produto_escalar_seq.c` | Questão 1 — produto escalar **sequencial** |
| `produto_escalar_par.c` | Questão 1 — produto escalar **paralelo** (pthreads) |
| `matmul_seq.c` | Questão 2 — multiplicação de matrizes **sequencial** |
| `matmul_par.c` | Questão 2 — multiplicação de matrizes **paralela** + **modo interativo** |
| `Makefile` | Compila tudo e gera os gráficos |
| `benchmark.sh` | Benchmark automático (**Linux/macOS**) → gera os CSVs |
| `benchmark.ps1` | Benchmark automático (**Windows/PowerShell**) → gera os CSVs |
| `gerar_graficos.py` | Lê os CSVs e gera os gráficos (PNG) + tabelas de speedup |
| `*_resultados.csv` | Resultados das medições (gerados pelo benchmark) |
| `graficos/` | Gráficos PNG (gerados pelo `gerar_graficos.py`) |

---

## ✅ Pré-requisitos

### Linux / macOS
- `gcc` e `make` (no Ubuntu/Debian: `sudo apt install build-essential`)
- Python 3 com matplotlib e pandas (só para os gráficos):
  ```bash
  pip install matplotlib pandas
  ```

### Windows
- **MinGW-w64 (gcc)** — o código usa `pthread.h` e `clock_gettime`, que **não existem no compilador da Microsoft (MSVC)**. Instale o gcc via [MSYS2](https://www.msys2.org/):
  ```
  pacman -S mingw-w64-ucrt-x86_64-gcc
  ```
  Confirme com `gcc --version` no PowerShell.
- Python 3 + matplotlib + pandas (para os gráficos): `pip install matplotlib pandas`

---

## 🔨 Como compilar

### Linux / macOS
```bash
make            # compila os 4 programas
make clean      # apaga os executáveis
```

### Windows (PowerShell)
O próprio `benchmark.ps1` compila sozinho. Se quiser compilar à mão:
```powershell
gcc -O2 -Wall produto_escalar_seq.c -o produto_escalar_seq.exe
gcc -O2 -Wall produto_escalar_par.c -o produto_escalar_par.exe -pthread
gcc -O2 -Wall matmul_seq.c          -o matmul_seq.exe
gcc -O2 -Wall matmul_par.c          -o matmul_par.exe -pthread
```

---

## ▶️ Como executar cada programa

> No Windows, acrescente `.exe` e use `.\` na frente (ex.: `.\produto_escalar_par.exe ...`).

### Questão 1 — Produto escalar
```bash
./produto_escalar_seq <tamanho_do_vetor>
./produto_escalar_par <tamanho_do_vetor> <num_threads>

# Exemplos:
./produto_escalar_seq 10000000
./produto_escalar_par 10000000 4
```

### Questão 2 — Multiplicação de matrizes

**Modo interativo** (foco em desempenho — pergunta os parâmetros e mostra o speedup):
```bash
./matmul_par
```
> Basta responder a dimensão e o número de threads (Enter aceita o valor padrão). Ele roda as duas versões e mostra `Ts`, `Tp`, **Speedup** e **Eficiência**. Para a demo, use dimensão entre 500 e 2000.

**Modo batch** (usado pelo benchmark; mede o tempo de uma execução):
```bash
./matmul_seq <dimensao>
./matmul_par <dimensao> <num_threads> [v]    # 'v' = verboso, mostra as threads

# Exemplos:
./matmul_seq 800
./matmul_par 800 4
./matmul_par 6 3 v        # matriz pequena, mostrando cada thread trabalhando
```

---

## 📊 Como gerar os resultados (benchmark + gráficos)

O **fluxo completo** é:

### Linux / macOS
```bash
make              # 1) compila
./benchmark.sh    # 2) roda tudo e gera os 2 CSVs
make graficos     # 3) gera os gráficos PNG na pasta graficos/
```

### Windows (PowerShell)
```powershell
# 1+2) compila (se preciso) e gera os 2 CSVs:
powershell -ExecutionPolicy Bypass -File .\benchmark.ps1

# 3) gera os gráficos:
python3 gerar_graficos.py
```

O benchmark gera:
- `produto_escalar_resultados.csv` (Questão 1)
- `matmul_resultados.csv` (Questão 2)

Colunas: `maquina, tamanho|dimensao, num_threads, ts, tp, speedup`.

> **Medição:** cada caso é executado 5 vezes e usamos a **média aparada** (descartamos o pior tempo) para reduzir o ruído do sistema. Feche outros programas durante a coleta.

---

## 🖥️ Rodando nos DOIS computadores (requisito do trabalho)

O trabalho pede medir em **2 computadores com quantidades de CPUs diferentes**. Como fazer:

1. Em **cada** computador: clone o repositório, compile e rode o benchmark.
   - O nome da máquina entra automaticamente no CSV (`hostname` no Linux, `COMPUTERNAME` no Windows).
2. **Renomeie** os CSVs de cada máquina para não sobrescrever, por exemplo:
   - PC 1: `pc1_q1.csv`, `pc1_q2.csv`
   - PC 2: `pc2_q1.csv`, `pc2_q2.csv`
3. Junte tudo nos gráficos passando os arquivos como argumento:
   ```bash
   python3 gerar_graficos.py pc1_q1.csv pc2_q1.csv pc1_q2.csv pc2_q2.csv
   ```
   O script gera um conjunto de gráficos **por máquina** e imprime as tabelas de speedup separadas por máquina.

> Dica: nos arrays de configuração do benchmark (`THREADS` no `.sh` / `$Threads` no `.ps1`), use valores compatíveis com o número de núcleos de cada PC.

---

## 📝 Notas técnicas

- **Por que não há `mutex` no cálculo?** Cada thread escreve em uma região exclusiva da saída (posição própria no produto escalar; linhas diferentes da matriz C). As entradas só são lidas. Logo, **não há condição de corrida**. (No modo verboso há um mutex apenas para organizar os `prints` na tela — ele não protege o cálculo.)
- **Produto escalar é *memory-bound*:** o ganho com threads satura cedo (limitado pela banda de memória).
- **Multiplicação de matrizes é *CPU-bound*:** escala melhor com threads, mas a eficiência cai em CPUs com núcleos de velocidades diferentes (ex.: Intel com P-cores e E-cores).

---

## 👥 Equipe

- _(preencher com os nomes do grupo)_
