/* ============================================================
 *  Trabalho Prático 1 - Sistemas Operacionais (UFAM)
 *  QUESTÃO 1 - Produto escalar entre dois vetores
 *  VERSÃO SEQUENCIAL (tradicional, sem threads)
 *
 *  Compilar:  gcc -O2 produto_escalar_seq.c -o produto_escalar_seq
 *  Executar:  ./produto_escalar_seq <tamanho_do_vetor>
 *  Exemplo:   ./produto_escalar_seq 10000000
 *
 *  Ideia: o produto escalar de A e B é a soma de A[i]*B[i] para
 *  todos os i. Aqui isso é feito por um único laço, em uma única
 *  thread (a thread principal do programa).
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Retorna o tempo atual, em segundos, com alta resolução.
   Usamos CLOCK_MONOTONIC porque ele não "anda para trás" caso
   o relógio do sistema seja ajustado durante a execução. */
double tempo_agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <tamanho_do_vetor>\n", argv[0]);
        return 1;
    }

    long n = atol(argv[1]);          /* quantidade de elementos */
    if (n <= 0) {
        printf("O tamanho do vetor deve ser maior que zero.\n");
        return 1;
    }

    /* Alocamos os vetores no heap (malloc) porque eles podem ter
       milhões de elementos e não caberiam na pilha. */
    double *a = malloc(n * sizeof(double));
    double *b = malloc(n * sizeof(double));
    if (a == NULL || b == NULL) {
        printf("Falha ao alocar memoria para os vetores.\n");
        return 1;
    }

    /* Preenche os vetores com valores. Mantemos simples e
       determinístico para o resultado ser sempre o mesmo. */
    for (long i = 0; i < n; i++) {
        a[i] = 1.0;        /* poderia ser aleatório; 1.0 e 2.0 */
        b[i] = 2.0;        /* deixam o resultado fácil de conferir */
    }

    printf("=== Produto escalar SEQUENCIAL ===\n");
    printf("Tamanho do vetor: %ld elementos\n", n);

    /* Medimos APENAS o cálculo, não a alocação nem o preenchimento,
       para a comparação com a versão paralela ser justa. */
    double t0 = tempo_agora();

    double resultado = 0.0;
    for (long i = 0; i < n; i++) {
        resultado += a[i] * b[i];
    }

    double t1 = tempo_agora();

    printf("Resultado do produto escalar: %.2f\n", resultado);
    printf("Tempo de execucao (Ts): %.6f segundos\n", t1 - t0);

    free(a);
    free(b);
    return 0;
}
