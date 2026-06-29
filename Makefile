# ============================================================
#  Trabalho Pratico 1 - Sistemas Operacionais (UFAM)
#  Makefile para compilar os programas das Questoes 1 e 2
#
#  Uso:
#    make          -> compila tudo
#    make graficos -> gera os graficos PNG a partir dos CSVs
#    make clean    -> apaga os executaveis
# ============================================================

CC      = gcc
CFLAGS  = -O2 -Wall
PTHREAD = -pthread

# Alvos que NAO sao arquivos (senao 'make graficos' acha que a pasta
# graficos/ ja' e' o resultado e nao roda nada).
.PHONY: all graficos clean

# "all" depende dos quatro executaveis
all: produto_escalar_seq produto_escalar_par matmul_seq matmul_par

# --- Questao 1: produto escalar ---
produto_escalar_seq: produto_escalar_seq.c
	$(CC) $(CFLAGS) produto_escalar_seq.c -o produto_escalar_seq

produto_escalar_par: produto_escalar_par.c
	$(CC) $(CFLAGS) produto_escalar_par.c -o produto_escalar_par $(PTHREAD)

# --- Questao 2: multiplicacao de matrizes ---
matmul_seq: matmul_seq.c
	$(CC) $(CFLAGS) matmul_seq.c -o matmul_seq

matmul_par: matmul_par.c
	$(CC) $(CFLAGS) matmul_par.c -o matmul_par $(PTHREAD)

# --- Graficos (Python + matplotlib) ---
graficos:
	python3 gerar_graficos.py

clean:
	rm -f produto_escalar_seq produto_escalar_par matmul_seq matmul_par
