/* ============================================================
 *  Trabalho Prático 1 - Sistemas Operacionais (UFAM)
 *  QUESTAO 2 - Aplicacao: MULTIPLICACAO DE MATRIZES
 *  VERSAO PARALELA (com pthreads)
 *
 *  Compilar:  gcc -O2 matmul_par.c -o matmul_par -pthread
 *
 *  DOIS MODOS DE USO:
 *
 *  1) MODO INTERATIVO (sem argumentos) -> abre um MENU de comandos:
 *         ./matmul_par
 *     O usuario escolhe a dimensao, o numero de threads e manda
 *     rodar. No modo paralelo o programa mostra CADA thread
 *     trabalhando (qual faixa de linhas ela calcula, progresso e
 *     quando termina). Serve para DEMONSTRAR o uso de threads.
 *
 *  2) MODO BATCH (com argumentos) -> usado pelo benchmark.sh:
 *         ./matmul_par <dimensao> <num_threads> [v]
 *         v = modo verboso (mostra as threads); sem 'v' = so mede tempo
 *     Exemplos:
 *         ./matmul_par 1000 4        (mede desempenho, silencioso)
 *         ./matmul_par 6 3 v         (demonstra as threads)
 *
 *  Estrategia de paralelizacao:
 *  - A matriz resultado C é dividida por LINHAS entre as threads.
 *  - Cada thread calcula um bloco de linhas de C, do inicio ao fim.
 *
 *  Por que NAO precisamos de semaforo/mutex no CALCULO?
 *  Cada thread escreve em linhas DIFERENTES de C. Duas threads nunca
 *  tocam a mesma posicao C[i][j]. As matrizes A e B sao apenas LIDAS
 *  (nunca alteradas), e varias threads podem ler a mesma memoria sem
 *  problema. Logo nao ha condicao de corrida no calculo.
 *
 *  (Usamos UM mutex apenas para os PRINTS do modo verboso nao se
 *   embaralharem na tela. Ele NAO protege o calculo -- so organiza a
 *   saida. Sem ele, varias threads escreveriam na mesma linha do
 *   terminal ao mesmo tempo e o texto sairia baguncado.)
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAX_THREADS 256

/* Mutex usado SO para organizar os prints do modo verboso.
   Nao tem relacao com o calculo das matrizes. */
static pthread_mutex_t print_mtx = PTHREAD_MUTEX_INITIALIZER;

/* Dados que cada thread recebe para trabalhar. */
typedef struct {
    int    id;            /* identificador da thread             */
    int    linha_inicio;  /* primeira linha de C que ela calcula */
    int    linha_fim;     /* ultima linha (exclusiva)            */
    int    n;             /* dimensao das matrizes               */
    double **A, **B, **C; /* ponteiros para as matrizes          */
    int    verboso;       /* 1 = imprimir status; 0 = silencioso */
} DadosThread;

double tempo_agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

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
    if (m == NULL) return;
    for (int i = 0; i < n; i++) free(m[i]);
    free(m);
}

/* Preenche A e B com valores deterministicos (semente fixa) para o
   resultado ser sempre o mesmo entre execucoes. */
void preenche_matrizes(int n, double **A, double **B) {
    srand(42);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
    }
}

/* Aloca A, B e C (NxN) e preenche A e B. Devolve 0 em caso de sucesso
   e -1 se faltar memoria. */
int prepara_matrizes(int n, double ***A, double ***B, double ***C) {
    *A = aloca_matriz(n);
    *B = aloca_matriz(n);
    *C = aloca_matriz(n);
    if (*A == NULL || *B == NULL || *C == NULL) return -1;
    preenche_matrizes(n, *A, *B);
    return 0;
}

/* Imprime uma matriz pequena (para visualizacao no menu). */
void imprime_matriz(const char *nome, double **M, int n) {
    printf("Matriz %s (%dx%d):\n", nome, n, n);
    for (int i = 0; i < n; i++) {
        printf("   ");
        for (int j = 0; j < n; j++) printf("%6.0f", M[i][j]);
        printf("\n");
    }
}

/* ---------- Funcao executada por CADA thread ---------- */
void *multiplica_bloco(void *arg) {
    DadosThread *d = (DadosThread *) arg;
    int total = d->linha_fim - d->linha_inicio;

    if (d->verboso) {
        pthread_mutex_lock(&print_mtx);
        printf("  [Thread %d] >> INICIOU | responsavel pelas linhas %d a %d (%d linha(s))\n",
               d->id, d->linha_inicio, d->linha_fim - 1, total);
        pthread_mutex_unlock(&print_mtx);
    }

    int feitas = 0;
    for (int i = d->linha_inicio; i < d->linha_fim; i++) {
        for (int j = 0; j < d->n; j++) {
            double soma = 0.0;
            for (int k = 0; k < d->n; k++) {
                soma += d->A[i][k] * d->B[k][j];
            }
            d->C[i][j] = soma;
        }
        feitas++;

        /* Mostra o progresso linha a linha so quando a matriz e
           pequena (senao, encheria a tela de texto). */
        if (d->verboso && d->n <= 12) {
            pthread_mutex_lock(&print_mtx);
            printf("       [Thread %d] calculou a linha %d   (%d/%d desta thread)\n",
                   d->id, i, feitas, total);
            pthread_mutex_unlock(&print_mtx);
        }
    }

    if (d->verboso) {
        pthread_mutex_lock(&print_mtx);
        printf("  [Thread %d] << TERMINOU (calculou %d linha(s))\n", d->id, total);
        pthread_mutex_unlock(&print_mtx);
    }
    return NULL;
}

/* ---------- Multiplicacao SEQUENCIAL (1 thread) ----------
   Devolve o tempo gasto, em segundos. */
double multiplica_sequencial(int n, double **A, double **B, double **C) {
    double t0 = tempo_agora();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double soma = 0.0;
            for (int k = 0; k < n; k++) soma += A[i][k] * B[k][j];
            C[i][j] = soma;
        }
    }
    return tempo_agora() - t0;
}

/* ---------- Multiplicacao PARALELA ----------
   Cria 'num_threads' threads, divide as linhas e espera todas.
   Devolve o tempo gasto, em segundos. */
double multiplica_paralelo(int n, int num_threads,
                           double **A, double **B, double **C, int verboso) {
    if (num_threads > n) num_threads = n;  /* no maximo 1 thread por linha */

    pthread_t   threads[MAX_THREADS];
    DadosThread dados[MAX_THREADS];

    int base  = n / num_threads;   /* linhas por thread (parte inteira) */
    int resto = n % num_threads;   /* sobra, distribuida nas primeiras  */

    if (verboso) {
        printf("\n  --- Mapa de divisao do trabalho (%d threads, %d linhas) ---\n",
               num_threads, n);
        int linha = 0;
        for (int t = 0; t < num_threads; t++) {
            int qtd = base + (t < resto ? 1 : 0);
            printf("    Thread %d  ->  linhas %d a %d  (%d linha(s))\n",
                   t, linha, linha + qtd - 1, qtd);
            linha += qtd;
        }
        printf("  ----------------------------------------------------------\n\n");
    }

    double t0 = tempo_agora();

    int linha = 0;
    for (int t = 0; t < num_threads; t++) {
        int qtd = base + (t < resto ? 1 : 0);
        dados[t].id           = t;
        dados[t].linha_inicio = linha;
        dados[t].linha_fim    = linha + qtd;
        dados[t].n            = n;
        dados[t].A            = A;
        dados[t].B            = B;
        dados[t].C            = C;
        dados[t].verboso      = verboso;
        pthread_create(&threads[t], NULL, multiplica_bloco, &dados[t]);
        linha += qtd;
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    return tempo_agora() - t0;
}

/* ==================================================================
 *  MODO BATCH  (usado pelo benchmark.sh)
 *  ./matmul_par <dimensao> <num_threads> [v]
 * ================================================================== */
int modo_batch(int argc, char *argv[]) {
    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int verboso = (argc >= 4 && argv[3][0] == 'v') ? 1 : 0;

    if (n <= 0 || num_threads <= 0 || num_threads > MAX_THREADS) {
        printf("Parametros invalidos (threads entre 1 e %d).\n", MAX_THREADS);
        return 1;
    }

    double **A, **B, **C;
    if (prepara_matrizes(n, &A, &B, &C) != 0) {
        printf("Falha ao alocar memoria para as matrizes.\n");
        return 1;
    }

    printf("=== Multiplicacao de matrizes PARALELA ===\n");
    printf("Dimensao: %d x %d\n", n, n);
    printf("Numero de threads: %d\n", num_threads > n ? n : num_threads);
    if (verboso) printf("Modo: VERBOSO (mostrando as threads)\n");

    double tp = multiplica_paralelo(n, num_threads, A, B, C, verboso);

    printf("C[0][0] = %.1f   C[%d][%d] = %.1f\n",
           C[0][0], n - 1, n - 1, C[n - 1][n - 1]);
    /* Esta linha e' lida pelo benchmark.sh -- NAO altere o formato. */
    printf("Tempo de execucao (Tp): %.6f segundos\n", tp);

    libera_matriz(A, n); libera_matriz(B, n); libera_matriz(C, n);
    return 0;
}

/* ==================================================================
 *  MODO INTERATIVO  (sem argumentos)
 *  Fluxo guiado, focado em DESEMPENHO:
 *    1) pergunta a dimensao e o numero de threads (Enter usa o padrao);
 *    2) roda a versao SEQUENCIAL e a PARALELA;
 *    3) mostra Ts, Tp e o SPEEDUP;
 *    4) pergunta se quer repetir com outros parametros.
 * ================================================================== */

/* Le um inteiro do teclado. Se o usuario so apertar Enter (linha vazia),
   devolve 'padrao'. Repete enquanto o valor estiver fora de [min, max]. */
int ler_inteiro(const char *rotulo, int padrao, int min, int max) {
    char buf[128];
    while (1) {
        printf("  %-34s [Enter = %d] : ", rotulo, padrao);
        fflush(stdout);
        if (fgets(buf, sizeof buf, stdin) == NULL) return padrao;  /* Ctrl+D */

        /* Linha vazia (so o '\n') -> usa o padrao. */
        if (buf[0] == '\n') return padrao;

        char *fim;
        long v = strtol(buf, &fim, 10);
        if (fim != buf && v >= min && v <= max) return (int) v;

        printf("    >> Valor invalido. Digite um numero entre %d e %d.\n", min, max);
    }
}

/* Pergunta "sim ou nao". Enter (vazio) conta como 'nao'. */
int pergunta_sim(const char *rotulo) {
    char buf[128];
    printf("\n%s (s/N) : ", rotulo);
    fflush(stdout);
    if (fgets(buf, sizeof buf, stdin) == NULL) return 0;
    return (buf[0] == 's' || buf[0] == 'S');
}

int modo_interativo(void) {
    int n = 800;          /* dimensao padrao  */
    int num_threads = 4;  /* threads padrao   */

    printf("\n============================================================\n");
    printf("  QUESTAO 2 - MULTIPLICACAO DE MATRIZES  (foco: DESEMPENHO)\n");
    printf("============================================================\n");
    printf("  Comparo a versao SEQUENCIAL com a PARALELA (pthreads) e\n");
    printf("  calculo o SPEEDUP (Sp = Ts / Tp). Basta responder abaixo.\n");
    printf("  (aperte Enter para aceitar o valor padrao sugerido)\n");
    printf("============================================================\n");

    do {
        printf("\n--- Parametros ---\n");
        n           = ler_inteiro("Dimensao N da matriz (NxN)", n, 1, 8000);
        num_threads = ler_inteiro("Numero de threads", num_threads, 1, MAX_THREADS);

        /* Multiplicar matrizes e' O(n^3) e a memoria cresce com n^2.
           Para N grande, avisa o custo e pede confirmacao. */
        double mem_mb = 3.0 * (double) n * n * sizeof(double) / (1024.0 * 1024.0);
        if (n > 1500) {
            printf("\n  ATENCAO: matriz %dx%d usa ~%.0f MB de RAM e o calculo e' O(n^3).\n",
                   n, n, mem_mb);
            printf("  Com N grande pode levar MUITO tempo. (Para demo, use N entre 500 e 2000.)\n");
            if (!pergunta_sim("  Continuar mesmo assim?")) continue;
        }

        int nt = num_threads > n ? n : num_threads;  /* no max. 1 thread por linha */

        double **A, **B, **C;
        if (prepara_matrizes(n, &A, &B, &C) != 0) {
            printf("\n  !! Falha ao alocar memoria para %dx%d. Tente uma dimensao menor.\n", n, n);
            continue;
        }

        printf("\n>> Matriz %dx%d, comparando 1 thread (sequencial) x %d thread(s)...\n",
               n, n, nt);

        /* Versao sequencial. */
        double ts = multiplica_sequencial(n, A, B, C);
        printf("   Sequencial pronto.\n");

        /* Versao paralela (silenciosa, para o tempo ser preciso). */
        double tp = multiplica_paralelo(n, num_threads, A, B, C, 0 /*silencioso*/);
        printf("   Paralelo pronto. (resultado confere: C[0][0] = %.0f)\n", C[0][0]);

        /* Resultados. */
        printf("\n   +--------------------------------------------------+\n");
        printf("   | Sequencial (Ts)          : %10.6f s        |\n", ts);
        printf("   | Paralelo   (Tp, %3d thr.) : %10.6f s        |\n", nt, tp);
        if (tp > 0) {
            double sp = ts / tp;
            printf("   | SPEEDUP  Sp = Ts / Tp     : %9.2f x         |\n", sp);
            printf("   | Eficiencia = Sp / threads : %8.0f %%          |\n",
                   100.0 * sp / nt);
        }
        printf("   +--------------------------------------------------+\n");
        if (tp > 0 && ts / tp < 1.0)
            printf("   (Sp < 1: a matriz e' pequena demais; o custo de criar as\n"
                   "    threads supera o ganho. Tente uma dimensao maior, ex.: 1000.)\n");

        libera_matriz(A, n); libera_matriz(B, n); libera_matriz(C, n);

    } while (pergunta_sim("Rodar de novo com outros parametros?"));

    printf("Encerrando. Ate mais!\n");
    return 0;
}

int main(int argc, char *argv[]) {
    /* Sem argumentos -> menu interativo (demonstracao).
       Com argumentos -> modo batch (benchmark.sh). */
    if (argc == 1) {
        return modo_interativo();
    }
    if (argc >= 3) {
        return modo_batch(argc, argv);
    }
    printf("Uso:\n");
    printf("  %s                         (modo INTERATIVO com menu)\n", argv[0]);
    printf("  %s <dimensao> <threads> [v]   (modo BATCH p/ benchmark)\n", argv[0]);
    return 1;
}
