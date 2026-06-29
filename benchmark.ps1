# ============================================================
#  Trabalho Pratico 1 - Sistemas Operacionais (UFAM)
#  Script de BENCHMARK automatico para WINDOWS (PowerShell)
#  Equivalente ao benchmark.sh (Linux). Questoes 1 e 2.
#
#  O que ele faz:
#   - Compila os 4 programas (se faltar algum .exe);
#   - Roda as versoes sequencial e paralela varias vezes;
#   - Usa a MEDIA APARADA (descarta o pior tempo de cada serie);
#   - Calcula o Speedup  Sp = Ts / Tp ;
#   - Mostra tabelas na tela e grava 2 arquivos .csv (ponto decimal).
#
#  PRE-REQUISITO: compilador MinGW-w64 (gcc) no PATH.
#    (O compilador da Microsoft/MSVC NAO serve: o codigo usa
#     pthread.h e clock_gettime, que sao do mundo POSIX/MinGW.)
#    Para conferir, abra o PowerShell e digite:  gcc --version
#
#  COMO RODAR (no PowerShell, dentro da pasta do projeto):
#     powershell -ExecutionPolicy Bypass -File .\benchmark.ps1
#
#  Rode SEPARADAMENTE em cada computador. O nome da maquina
#  ($env:COMPUTERNAME) entra no CSV; depois e' so juntar os arquivos.
# ============================================================

# ----------------- CONFIGURACAO (edite a vontade) -----------------

# Tamanhos do vetor (Questao 1). Memoria ~ tamanho * 16 bytes.
$TamanhosVetor = @(1000000, 5000000, 10000000, 20000000)

# Dimensoes das matrizes NxN (Questao 2). Memoria ~ N*N*24 bytes.
$DimensoesMatriz = @(200, 400, 600, 800)

# Quantidades de threads (ate o num. de nucleos da maquina):
$Threads = @(1, 2, 4, 8)

# Quantas vezes repetir cada medicao. Usamos MEDIA APARADA: o pior
# tempo (mais alto, geralmente "sujo" por interferencia do sistema)
# e' descartado. Por isso convem ter pelo menos 4-5 repeticoes.
$Repeticoes = 5

# Nome desta maquina (vai para o CSV).
$Maquina = $env:COMPUTERNAME

# ------------------------------------------------------------------

# Cultura invariante: garante o PONTO "." como separador decimal,
# mesmo num Windows configurado em portugues (que usa virgula).
$inv = [System.Globalization.CultureInfo]::InvariantCulture

Write-Host "Maquina: $Maquina   |   Nucleos logicos: $env:NUMBER_OF_PROCESSORS"
Write-Host ""

# ----------------- Compilacao (se necessario) -----------------
$execs = @(
    "produto_escalar_seq.exe",
    "produto_escalar_par.exe",
    "matmul_seq.exe",
    "matmul_par.exe"
)
$faltando = $execs | Where-Object { -not (Test-Path $_) }

if ($faltando.Count -gt 0) {
    if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
        Write-Host "ERRO: 'gcc' nao encontrado no PATH." -ForegroundColor Red
        Write-Host "Instale o MinGW-w64 (ex.: via MSYS2) e garanta que 'gcc' funcione." -ForegroundColor Red
        exit 1
    }
    Write-Host "Compilando os programas com gcc..."
    & gcc -O2 -Wall produto_escalar_seq.c -o produto_escalar_seq.exe
    if ($LASTEXITCODE -ne 0) { Write-Host "Falha ao compilar produto_escalar_seq. Abortando." -ForegroundColor Red; exit 1 }
    & gcc -O2 -Wall produto_escalar_par.c -o produto_escalar_par.exe -pthread
    if ($LASTEXITCODE -ne 0) { Write-Host "Falha ao compilar produto_escalar_par. Abortando." -ForegroundColor Red; exit 1 }
    & gcc -O2 -Wall matmul_seq.c          -o matmul_seq.exe
    if ($LASTEXITCODE -ne 0) { Write-Host "Falha ao compilar matmul_seq. Abortando." -ForegroundColor Red; exit 1 }
    & gcc -O2 -Wall matmul_par.c          -o matmul_par.exe -pthread
    if ($LASTEXITCODE -ne 0) { Write-Host "Falha ao compilar matmul_par. Abortando." -ForegroundColor Red; exit 1 }
    Write-Host ""
}

# ----------------- Funcoes auxiliares -----------------

# Roda um programa $Repeticoes vezes e devolve o tempo (media aparada).
# Le a linha "Tempo de execucao (...): <numero> segundos" da saida.
# Devolve $null (e mostra o motivo) se alguma execucao falhar.
function Roda-E-Mede {
    param(
        [string]   $Exe,
        [string[]] $ProgArgs   # nao usar '$Args': e' variavel reservada do PowerShell
    )
    $tempos = New-Object System.Collections.Generic.List[double]
    for ($i = 1; $i -le $Repeticoes; $i++) {
        $saida = & ".\$Exe" @ProgArgs 2>&1 | Out-String
        $linha = ($saida -split "`n") | Where-Object { $_ -match "Tempo de execucao" } | Select-Object -First 1
        if (-not $linha) {
            Write-Host "   !! FALHA ao executar:  $Exe $ProgArgs" -ForegroundColor Red
            Write-Host "      Saida do programa:" -ForegroundColor Red
            ($saida -split "`n") | ForEach-Object { Write-Host "      | $_" }
            return $null
        }
        # tokens: Tempo de execucao (Ts): <numero> segundos  -> indice 4
        $tok = ($linha.Trim() -split "\s+")
        $tempos.Add([double]::Parse($tok[4], $inv))
    }

    # Media aparada: com 3+ amostras, descarta UMA ocorrencia do maior.
    if ($tempos.Count -ge 3) {
        $piorIdx = 0
        for ($j = 1; $j -lt $tempos.Count; $j++) {
            if ($tempos[$j] -gt $tempos[$piorIdx]) { $piorIdx = $j }
        }
        $soma = 0.0; $n = 0
        for ($j = 0; $j -lt $tempos.Count; $j++) {
            if ($j -ne $piorIdx) { $soma += $tempos[$j]; $n++ }
        }
        return $soma / $n
    } else {
        $soma = 0.0
        foreach ($t in $tempos) { $soma += $t }
        return $soma / $tempos.Count
    }
}

# Formata um double com ponto decimal (para o CSV e as tabelas).
function Fmt6 { param([double]$x) return $x.ToString("F6", $inv) }
function Fmt2 { param([double]$x) return $x.ToString("F2", $inv) }

# ==================================================================
#  QUESTAO 1 - PRODUTO ESCALAR
# ==================================================================
$Csv1 = "produto_escalar_resultados.csv"
"maquina,tamanho,num_threads,ts,tp,speedup" | Out-File -Encoding ascii $Csv1

Write-Host "############################################################"
Write-Host "#  QUESTAO 1 - PRODUTO ESCALAR   (maquina: $Maquina)"
Write-Host "############################################################"

foreach ($tam in $TamanhosVetor) {
    $ts = Roda-E-Mede "produto_escalar_seq.exe" @("$tam")
    Write-Host ""
    Write-Host (">> Vetor de $tam elementos   |   Ts (sequencial) = " + (Fmt6 $ts))
    "{0,-10} {1,-14} {2,-10}" -f "threads", "Tp (paralelo)", "Speedup" | Write-Host
    "{0,-10} {1,-14} {2,-10}" -f "-------", "-------------", "-------" | Write-Host

    foreach ($th in $Threads) {
        $tp = Roda-E-Mede "produto_escalar_par.exe" @("$tam", "$th")
        $sp = if ($tp -gt 0) { $ts / $tp } else { 0 }
        "{0,-10} {1,-14} {2,-10}" -f "$th", (Fmt6 $tp), (Fmt2 $sp) | Write-Host
        "$Maquina,$tam,$th,$(Fmt6 $ts),$(Fmt6 $tp),$(Fmt2 $sp)" | Out-File -Append -Encoding ascii $Csv1
    }
}

# ==================================================================
#  QUESTAO 2 - MULTIPLICACAO DE MATRIZES
# ==================================================================
$Csv2 = "matmul_resultados.csv"
"maquina,dimensao,num_threads,ts,tp,speedup" | Out-File -Encoding ascii $Csv2

Write-Host ""
Write-Host "############################################################"
Write-Host "#  QUESTAO 2 - MULTIPLICACAO DE MATRIZES   (maquina: $Maquina)"
Write-Host "############################################################"

foreach ($dim in $DimensoesMatriz) {
    $ts = Roda-E-Mede "matmul_seq.exe" @("$dim")
    Write-Host ""
    Write-Host (">> Matriz ${dim}x${dim}   |   Ts (sequencial) = " + (Fmt6 $ts))
    "{0,-10} {1,-14} {2,-10}" -f "threads", "Tp (paralelo)", "Speedup" | Write-Host
    "{0,-10} {1,-14} {2,-10}" -f "-------", "-------------", "-------" | Write-Host

    foreach ($th in $Threads) {
        $tp = Roda-E-Mede "matmul_par.exe" @("$dim", "$th")
        $sp = if ($tp -gt 0) { $ts / $tp } else { 0 }
        "{0,-10} {1,-14} {2,-10}" -f "$th", (Fmt6 $tp), (Fmt2 $sp) | Write-Host
        "$Maquina,$dim,$th,$(Fmt6 $ts),$(Fmt6 $tp),$(Fmt2 $sp)" | Out-File -Append -Encoding ascii $Csv2
    }
}

Write-Host ""
Write-Host "############################################################"
Write-Host "#  Concluido!  Resultados em:"
Write-Host "#    - $Csv1"
Write-Host "#    - $Csv2"
Write-Host "############################################################"
