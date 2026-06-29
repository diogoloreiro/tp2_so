/* ============================================================
 *  Trabalho Prático 1 - Sistemas Operacionais (UFAM)
 *  QUESTAO 2 - Aplicacao: MULTIPLICACAO DE MATRIZES
 *  VERSAO SEQUENCIAL (tradicional, sem threads)
 *
 *  Compilar:  gcc -O2 matmul_seq.c -o matmul_seq
 *  Executar:  ./matmul_seq <dimensao>
 *  Exemplo:   ./matmul_seq 1000
 *
 *  Multiplicamos duas matrizes quadradas A e B (dimensao x dimensao),
 *  gerando C = A x B. Cada elemento C[i][j] é o produto escalar da
 *  linha i de A pela coluna j de B  ->  C[i][j] = soma_k A[i][k]*B[k][j].
 *  (Repare como isto liga diretamente com a Questao 1!)
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double tempo_agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* Aloca uma matriz n x n como um vetor de ponteiros de linha.
   Assim podemos acessar de forma natural: M[i][j]. */
double **aloca_matriz(int n) {
    double **m = malloc(n * sizeof(double *));
    if (m == NULL) return NULL;
    for (int i = 0; i < n; i++) {
        m[i] = malloc(n * sizeof(double));
        if (m[i] == NULL) return NULL;
    }
    return m;
}

void libera_matriz(double **m, int n) {
    for (int i = 0; i < n; i++) free(m[i]);
    free(m);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <dimensao>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        printf("A dimensao deve ser maior que zero.\n");
        return 1;
    }

    double **A = aloca_matriz(n);
    double **B = aloca_matriz(n);
    double **C = aloca_matriz(n);
    if (A == NULL || B == NULL || C == NULL) {
        printf("Falha ao alocar memoria para as matrizes.\n");
        return 1;
    }

    /* Preenche A e B com valores aleatorios entre 0 e 9. */
    srand(42);   /* semente fixa: o resultado é sempre o mesmo */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
    }

    printf("=== Multiplicacao de matrizes SEQUENCIAL ===\n");
    printf("Dimensao: %d x %d\n", n, n);

    /* Medimos apenas o calculo da multiplicacao. */
    double t0 = tempo_agora();

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double soma = 0.0;
            for (int k = 0; k < n; k++) {
                soma += A[i][k] * B[k][j];
            }
            C[i][j] = soma;
        }
    }

    double t1 = tempo_agora();

    /* Imprime um "canto" da matriz so para conferir que houve calculo. */
    printf("C[0][0] = %.1f   C[%d][%d] = %.1f\n",
           C[0][0], n - 1, n - 1, C[n - 1][n - 1]);
    printf("Tempo de execucao (Ts): %.6f segundos\n", t1 - t0);

    libera_matriz(A, n);
    libera_matriz(B, n);
    libera_matriz(C, n);
    return 0;
}
