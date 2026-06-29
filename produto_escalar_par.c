/* ============================================================
 *  Trabalho Prático 1 - Sistemas Operacionais (UFAM)
 *  QUESTÃO 1 - Produto escalar entre dois vetores
 *  VERSÃO PARALELA (com pthreads)
 *
 *  Compilar:  gcc -O2 produto_escalar_par.c -o produto_escalar_par -pthread
 *  Executar:  ./produto_escalar_par <tamanho_do_vetor> <num_threads>
 *  Exemplo:   ./produto_escalar_par 10000000 4
 *
 *  Estratégia de paralelização:
 *  - O vetor é dividido em "fatias" (chunks), uma para cada thread.
 *  - Cada thread calcula a soma parcial da sua fatia.
 *  - A thread principal soma as parciais no final.
 *
 *  Por que NÃO precisamos de semáforo/mutex aqui?
 *  Porque cada thread escreve o seu resultado em uma posição
 *  exclusiva (o campo "parcial" da SUA struct). Não há duas
 *  threads escrevendo na mesma variável, logo não há condição
 *  de corrida. A soma final das parciais é feita por uma única
 *  thread (a principal), depois que todas terminaram.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAX_THREADS 256

/* Tudo o que UMA thread precisa para fazer o seu trabalho. */
typedef struct {
    int    id;          /* identificador da thread: 0, 1, 2, ...   */
    long   inicio;      /* primeiro indice da fatia (inclusivo)    */
    long   fim;         /* ultimo indice da fatia (exclusivo)      */
    double *a;          /* ponteiro para o vetor A                 */
    double *b;          /* ponteiro para o vetor B                 */
    double parcial;     /* SAIDA: soma parcial calculada pela thread */
} DadosThread;

double tempo_agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* Funcao executada por cada thread: percorre a sua fatia do vetor
   e acumula a soma dos produtos A[i]*B[i]. */
void *calcula_parcial(void *arg) {
    DadosThread *d = (DadosThread *) arg;

    double soma = 0.0;
    for (long i = d->inicio; i < d->fim; i++) {
        soma += d->a[i] * d->b[i];
    }
    d->parcial = soma;   /* grava na posicao EXCLUSIVA desta thread */

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <tamanho_do_vetor> <num_threads>\n", argv[0]);
        return 1;
    }

    long n = atol(argv[1]);
    int  num_threads = atoi(argv[2]);

    if (n <= 0 || num_threads <= 0 || num_threads > MAX_THREADS) {
        printf("Parametros invalidos (threads entre 1 e %d).\n", MAX_THREADS);
        return 1;
    }
    if (num_threads > n) num_threads = (int) n;  /* nao faz sentido ter mais threads que elementos */

    double *a = malloc(n * sizeof(double));
    double *b = malloc(n * sizeof(double));
    if (a == NULL || b == NULL) {
        printf("Falha ao alocar memoria para os vetores.\n");
        return 1;
    }

    for (long i = 0; i < n; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
    }

    printf("=== Produto escalar PARALELO ===\n");
    printf("Tamanho do vetor: %ld elementos\n", n);
    printf("Numero de threads: %d\n", num_threads);

    pthread_t   threads[MAX_THREADS];
    DadosThread dados[MAX_THREADS];

    /* Divisao do trabalho: cada thread recebe um bloco de tamanho
       parecido. O resto (n % num_threads) é distribuido entre as
       primeiras threads, para ninguem ficar sobrecarregado. */
    long base  = n / num_threads;
    long resto = n % num_threads;

    /* Comecamos a medir o tempo: criar + executar + juntar as threads. */
    double t0 = tempo_agora();

    long inicio = 0;
    for (int t = 0; t < num_threads; t++) {
        long tamanho = base + (t < resto ? 1 : 0);  /* +1 para as primeiras */

        dados[t].id     = t;
        dados[t].inicio = inicio;
        dados[t].fim    = inicio + tamanho;
        dados[t].a      = a;
        dados[t].b      = b;
        dados[t].parcial = 0.0;

        pthread_create(&threads[t], NULL, calcula_parcial, &dados[t]);

        inicio += tamanho;
    }

    /* Espera todas as threads terminarem e soma as parciais. */
    double resultado = 0.0;
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
        resultado += dados[t].parcial;
    }

    double t1 = tempo_agora();

    printf("Resultado do produto escalar: %.2f\n", resultado);
    printf("Tempo de execucao (Tp): %.6f segundos\n", t1 - t0);

    free(a);
    free(b);
    return 0;
}
