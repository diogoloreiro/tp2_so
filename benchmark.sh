#!/bin/bash
# ============================================================
#  Trabalho Prático 1 - Sistemas Operacionais (UFAM)
#  Script de BENCHMARK automatico (Questoes 1 e 2)  -- v2
#
#  O que ele faz:
#   - Roda as versoes sequencial e paralela varias vezes;
#   - Tira a MEDIA dos tempos (reduz o "ruido" do sistema);
#   - Calcula o Speedup  Sp = Ts / Tp ;
#   - Mostra tabelas no terminal e grava 2 arquivos .csv.
#
#  Se UMA execucao falhar (ex: falta de memoria), o script
#  MOSTRA o motivo e marca a celula como "FALHOU" -- nunca
#  mais aparece "nan" sem explicacao.
#
#  Uso:
#     chmod +x benchmark.sh      (so na primeira vez)
#     ./benchmark.sh
#
#  Rode SEPARADAMENTE em cada computador (o nome da maquina
#  entra no CSV, entao depois e' so juntar os dois arquivos).
# ============================================================

# Forca o ponto "." como separador decimal, em qualquer maquina.
export LC_ALL=C

# ----------------- CONFIGURACAO (edite a vontade) -----------------

# Tamanhos do vetor (Questao 1). CUIDADO COM MEMORIA:
#   cada tamanho usa ~ tamanho * 16 bytes  (dois vetores de double).
#   10.000.000 -> ~160 MB | 20.000.000 -> ~320 MB | 50.000.000 -> ~800 MB
TAMANHOS_VETOR=(1000000 5000000 10000000 20000000)

# Dimensoes das matrizes NxN (Questao 2). Memoria ~ N*N*24 bytes:
#   500 -> ~6 MB | 1000 -> ~24 MB | 1500 -> ~54 MB  (bem leve)
DIMENSOES_MATRIZ=(200 400 600 800)

# Quantidades de threads (use valores ate o num. de nucleos da maquina):
THREADS=(1 2 4 8)

# Quantas vezes repetir cada medicao. Usamos a MEDIA APARADA: o pior
# tempo (o mais alto, geralmente "sujo" por interferencia do sistema)
# e' descartado, e a media e' feita com os demais. Por isso convem ter
# pelo menos 4-5 repeticoes.
REPETICOES=5

# Nome desta maquina (vai para o CSV). Troque se os dois PCs
# tiverem o mesmo hostname (ex: MAQUINA="pc_lab").
MAQUINA="$(hostname)"

# ------------------------------------------------------------------

# Compila se faltar algum executavel.
if [ ! -f ./produto_escalar_seq ] || [ ! -f ./produto_escalar_par ] || \
   [ ! -f ./matmul_seq ] || [ ! -f ./matmul_par ]; then
    echo "Executaveis nao encontrados. Rodando 'make'..."
    make || { echo "Falha ao compilar. Abortando."; exit 1; }
    echo
fi

# Roda um comando REPETICOES vezes e devolve o tempo (MEDIA APARADA:
# descarta o pior tempo e faz a media dos demais).
# Se o programa nao imprimir o tempo (ex: erro de memoria), mostra
# a saida real do programa e devolve "FALHOU".
roda_e_mede() {
    local tempos="" i saida t
    for ((i = 1; i <= REPETICOES; i++)); do
        saida="$("$@" 2>&1)"                      # captura stdout E stderr
        t="$(printf '%s\n' "$saida" \
             | grep "Tempo de execucao" | awk '{print $5}')"
        if [ -z "$t" ]; then
            echo "   !! FALHA ao executar:  $*" >&2
            echo "      Saida do programa:" >&2
            printf '%s\n' "$saida" | sed 's/^/      | /' >&2
            echo "FALHOU"
            return 1
        fi
        tempos="${tempos}${t}"$'\n'
    done
    # Media aparada: com 3+ amostras, descarta UMA ocorrencia do maior
    # tempo (o "outlier" lento) e faz a media do resto. Com menos
    # amostras, faz a media simples.
    printf '%s\n' "$tempos" \
        | awk 'NF {v[c++] = $1}
               END {
                   if (c == 0) { printf "FALHOU"; exit }
                   if (c >= 3) {
                       mi = 0;                       # indice do pior (maior) tempo
                       for (i = 1; i < c; i++) if (v[i] > v[mi]) mi = i;
                       s = 0; k = 0;
                       for (i = 0; i < c; i++) if (i != mi) { s += v[i]; k++ }
                       printf "%.6f", s / k;
                   } else {
                       s = 0;
                       for (i = 0; i < c; i++) s += v[i];
                       printf "%.6f", s / c;
                   }
               }'
}

# speedup = ts / tp. Se algum valor nao for numero, devolve "-".
calc_speedup() {
    awk -v ts="$1" -v tp="$2" '
        BEGIN {
            if (ts+0 == ts && tp+0 == tp && tp > 0) printf "%.2f", ts/tp;
            else printf "-";
        }'
}

# ==================================================================
#  QUESTAO 1 - PRODUTO ESCALAR
# ==================================================================
CSV1="produto_escalar_resultados.csv"
echo "maquina,tamanho,num_threads,ts,tp,speedup" > "$CSV1"

echo "############################################################"
echo "#  QUESTAO 1 - PRODUTO ESCALAR   (maquina: $MAQUINA)"
echo "############################################################"

for tam in "${TAMANHOS_VETOR[@]}"; do
    ts="$(roda_e_mede ./produto_escalar_seq "$tam")"

    echo
    echo ">> Vetor de $tam elementos   |   Ts (sequencial) = ${ts}"
    printf "   %-10s %-14s %-10s\n" "threads" "Tp (paralelo)" "Speedup"
    printf "   %-10s %-14s %-10s\n" "-------" "-------------" "-------"

    for th in "${THREADS[@]}"; do
        tp="$(roda_e_mede ./produto_escalar_par "$tam" "$th")"
        sp="$(calc_speedup "$ts" "$tp")"
        printf "   %-10s %-14s %-10s\n" "$th" "$tp" "$sp"
        echo "$MAQUINA,$tam,$th,$ts,$tp,$sp" >> "$CSV1"
    done
done

# ==================================================================
#  QUESTAO 2 - MULTIPLICACAO DE MATRIZES
# ==================================================================
CSV2="matmul_resultados.csv"
echo "maquina,dimensao,num_threads,ts,tp,speedup" > "$CSV2"

echo
echo "############################################################"
echo "#  QUESTAO 2 - MULTIPLICACAO DE MATRIZES   (maquina: $MAQUINA)"
echo "############################################################"

for dim in "${DIMENSOES_MATRIZ[@]}"; do
    ts="$(roda_e_mede ./matmul_seq "$dim")"

    echo
    echo ">> Matriz ${dim}x${dim}   |   Ts (sequencial) = ${ts}"
    printf "   %-10s %-14s %-10s\n" "threads" "Tp (paralelo)" "Speedup"
    printf "   %-10s %-14s %-10s\n" "-------" "-------------" "-------"

    for th in "${THREADS[@]}"; do
        tp="$(roda_e_mede ./matmul_par "$dim" "$th")"
        sp="$(calc_speedup "$ts" "$tp")"
        printf "   %-10s %-14s %-10s\n" "$th" "$tp" "$sp"
        echo "$MAQUINA,$dim,$th,$ts,$tp,$sp" >> "$CSV2"
    done
done

echo
echo "############################################################"
echo "#  Concluido!  Resultados em:"
echo "#    - $CSV1"
echo "#    - $CSV2"
echo "############################################################"
