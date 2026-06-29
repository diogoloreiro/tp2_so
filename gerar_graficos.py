#!/usr/bin/env python3
# ============================================================
#  Trabalho Pratico 1 - Sistemas Operacionais (UFAM)
#  Gera os GRAFICOS e TABELAS a partir dos CSVs do benchmark.
#
#  Le os arquivos:
#     - produto_escalar_resultados.csv   (Questao 1)
#     - matmul_resultados.csv            (Questao 2)
#
#  Produz (na pasta 'graficos/'):
#     - <maquina>_q1_tempo.png      tempo x tamanho (Ts vs Tp por nr de threads)
#     - <maquina>_q1_speedup.png    speedup x nr de threads (uma curva por tamanho)
#     - <maquina>_q2_tempo.png      tempo x dimensao
#     - <maquina>_q2_speedup.png    speedup x nr de threads
#  e imprime as tabelas de speedup no terminal.
#
#  COMO USAR:
#     python3 gerar_graficos.py
#  ou, juntando os CSVs de DUAS maquinas:
#     python3 gerar_graficos.py pc1_q1.csv pc2_q1.csv pc1_q2.csv pc2_q2.csv
#
#  Requisitos:  pip install pandas matplotlib
# ============================================================

import os
import sys
import glob

import matplotlib
matplotlib.use("Agg")          # nao precisa de tela; salva PNG direto
import matplotlib.pyplot as plt
import pandas as pd

PASTA_SAIDA = "graficos"


def carrega_csvs(arquivos, coluna_tamanho):
    """Le e junta varios CSVs que tenham a coluna indicada
    ('tamanho' p/ Questao 1 ou 'dimensao' p/ Questao 2)."""
    quadros = []
    for caminho in arquivos:
        if not os.path.isfile(caminho):
            continue
        df = pd.read_csv(caminho)
        if coluna_tamanho not in df.columns:
            continue
        # converte numeros; valores tipo "FALHOU"/"-" viram NaN e somem
        for col in ("ts", "tp", "speedup", "num_threads", coluna_tamanho):
            df[col] = pd.to_numeric(df[col], errors="coerce")
        df = df.dropna(subset=["ts", "tp", "num_threads", coluna_tamanho])
        quadros.append(df)
    if not quadros:
        return None
    return pd.concat(quadros, ignore_index=True)


def grafico_tempo(df, coluna_tamanho, titulo, rotulo_x, prefixo):
    """Tempo de execucao x tamanho: 1 curva sequencial (Ts) +
    1 curva paralela por quantidade de threads (Tp)."""
    for maquina, dm in df.groupby("maquina"):
        plt.figure(figsize=(8, 5))
        tamanhos = sorted(dm[coluna_tamanho].unique())

        # Curva SEQUENCIAL (Ts e' o mesmo para qualquer nr de threads;
        # pegamos a media por tamanho).
        ts = [dm[dm[coluna_tamanho] == t]["ts"].mean() for t in tamanhos]
        plt.plot(tamanhos, ts, marker="o", linewidth=2.2,
                 color="black", label="Sequencial (Ts)")

        # Uma curva PARALELA por quantidade de threads.
        for th in sorted(dm["num_threads"].unique()):
            sub = dm[dm["num_threads"] == th]
            tp = [sub[sub[coluna_tamanho] == t]["tp"].mean() for t in tamanhos]
            plt.plot(tamanhos, tp, marker="s", linestyle="--",
                     label=f"Paralelo - {int(th)} thread(s) (Tp)")

        plt.title(f"{titulo}\nMaquina: {maquina}")
        plt.xlabel(rotulo_x)
        plt.ylabel("Tempo de execucao (segundos)")
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.tight_layout()
        nome = os.path.join(PASTA_SAIDA, f"{maquina}_{prefixo}_tempo.png")
        plt.savefig(nome, dpi=120)
        plt.close()
        print(f"  [ok] {nome}")


def grafico_speedup(df, coluna_tamanho, titulo, prefixo):
    """Speedup x nr de threads: uma curva por tamanho, mais a
    referencia de speedup ideal (linear)."""
    for maquina, dm in df.groupby("maquina"):
        plt.figure(figsize=(8, 5))
        threads = sorted(dm["num_threads"].unique())

        for t in sorted(dm[coluna_tamanho].unique()):
            sub = dm[dm[coluna_tamanho] == t].sort_values("num_threads")
            sp = sub["ts"].values / sub["tp"].values
            plt.plot(sub["num_threads"], sp, marker="o",
                     label=f"{int(t)}")

        # Linha de speedup IDEAL (Sp = p) como referencia.
        plt.plot(threads, threads, linestyle=":", color="gray",
                 label="Ideal (linear)")

        plt.title(f"Speedup x nr de threads - {titulo}\nMaquina: {maquina}")
        plt.xlabel("Numero de threads")
        plt.ylabel("Speedup  (Sp = Ts / Tp)")
        plt.grid(True, alpha=0.3)
        plt.legend(title=("tamanho" if coluna_tamanho == "tamanho" else "dimensao"))
        plt.tight_layout()
        nome = os.path.join(PASTA_SAIDA, f"{maquina}_{prefixo}_speedup.png")
        plt.savefig(nome, dpi=120)
        plt.close()
        print(f"  [ok] {nome}")


def tabela_speedup(df, coluna_tamanho, titulo):
    """Imprime no terminal uma tabela de speedup (tamanho x threads),
    por maquina."""
    print(f"\n================ TABELA DE SPEEDUP - {titulo} ================")
    for maquina, dm in df.groupby("maquina"):
        print(f"\n  Maquina: {maquina}")
        dm = dm.copy()
        dm["sp"] = dm["ts"] / dm["tp"]
        tabela = dm.pivot_table(index=coluna_tamanho,
                                columns="num_threads",
                                values="sp", aggfunc="mean")
        # cabecalho
        print("    " + f"{coluna_tamanho:>12}", end="")
        for th in tabela.columns:
            print(f"  {int(th):>2} thr", end="")
        print()
        for tam, linha in tabela.iterrows():
            print(f"    {int(tam):>12}", end="")
            for th in tabela.columns:
                v = linha[th]
                print(f"  {v:6.2f}" if pd.notna(v) else "     -", end="")
            print()


def main():
    args = sys.argv[1:]
    # separa os CSVs informados em "Questao 1" e "Questao 2" pelas colunas.
    if args:
        arquivos_q1 = [a for a in args if "tamanho" in pd.read_csv(a, nrows=0).columns]
        arquivos_q2 = [a for a in args if "dimensao" in pd.read_csv(a, nrows=0).columns]
    else:
        arquivos_q1 = glob.glob("produto_escalar_resultados.csv")
        arquivos_q2 = glob.glob("matmul_resultados.csv")

    os.makedirs(PASTA_SAIDA, exist_ok=True)

    # ---------------- Questao 1 ----------------
    df1 = carrega_csvs(arquivos_q1, "tamanho")
    if df1 is not None:
        print("\n>> Questao 1 - Produto escalar")
        grafico_tempo(df1, "tamanho",
                      "Produto escalar - tempo x tamanho do vetor",
                      "Tamanho do vetor (nr de elementos)", "q1")
        grafico_speedup(df1, "tamanho", "Produto escalar", "q1")
        tabela_speedup(df1, "tamanho", "QUESTAO 1 (produto escalar)")
    else:
        print(">> Questao 1: nenhum CSV valido encontrado (pulei).")

    # ---------------- Questao 2 ----------------
    df2 = carrega_csvs(arquivos_q2, "dimensao")
    if df2 is not None:
        print("\n>> Questao 2 - Multiplicacao de matrizes")
        grafico_tempo(df2, "dimensao",
                      "Multiplicacao de matrizes - tempo x dimensao",
                      "Dimensao da matriz (N de NxN)", "q2")
        grafico_speedup(df2, "dimensao", "Multiplicacao de matrizes", "q2")
        tabela_speedup(df2, "dimensao", "QUESTAO 2 (multiplicacao de matrizes)")
    else:
        print(">> Questao 2: nenhum CSV valido encontrado (pulei).")

    print(f"\nPronto! Os graficos PNG estao na pasta '{PASTA_SAIDA}/'.")


if __name__ == "__main__":
    main()
