#!/usr/bin/env bash
# =============================================================================
#  grid_search.sh  --  varredura de parametros do GA para o EVRP
#
#  Estrategia: para cada combinacao de parametros, gera um build isolado
#  (recompila em ~2s), roda TRIALS execucoes em cada instancia, e grava
#  o resultado (media, desvio, melhor, pior, gap%) num CSV, linha a linha.
#
#  - Log incremental: cada config e escrita assim que termina, entao se a
#    maquina cair/dormir voce nao perde o que ja rodou.
#  - Resumivel: rodar de novo pula as combinacoes ja concluidas.
#  - Nao mexe nos seus fontes: tudo acontece numa pasta de build separada.
#
#  Uso tipico (deixar rodando a noite):
#     nohup ./grid_search.sh > grid_stdout.log 2>&1 &
#     tail -f grid_resultados/grid.log        # acompanhar
# =============================================================================
set -uo pipefail

# ----------------------------- CONFIGURACAO ----------------------------------
SRC="."                              # pasta com os .cpp/.hpp
INSTDIR="."                          # pasta com os arquivos .evrp
OUTDIR="grid_resultados"             # onde vao os resultados
INSTANCIAS=(E-n22-k4 E-n76-k7)       # nomes (sem .evrp) das instancias a testar

TRIALS=20                            # execucoes por config (a competicao usa 20)
EVAL_MULT=25000                      # orcamento: 25000 = cheio. Reduza (ex.: 3000)
                                     # para uma varredura exploratoria mais rapida.
#
#  TEMPO ESTIMADO (medido em 1 nucleo, -O2, nas 2 instancias):
#    pior caso ~7.4s/trial (E-n76) + ~1.3s/trial (E-n22) por config.
#    O grid default (72 configs) roda em ~1h com TRIALS=10 e ~2h com TRIALS=20,
#    no orcamento CHEIO. Sobra folga p/ expandir muito as listas abaixo.

# ------------------------------- O GRID --------------------------------------
# Edite estas listas a vontade. O total de configs = produto dos tamanhos.
POP_LIST=(50 100 200)                # POP_SIZE
KTORN_LIST=(2 4 10)                  # K_TORNEIO
PCROSS_LIST=(0.7 0.9)                # PROB_CROSSOVER
PMUT_LIST=(0.7 0.9)                  # PROB_MUTACAO
ESTAG_LIST=(1000 10000)                    # LIMITE_ESTAGNACAO
OPT2_LIST=(true)                     # FL_OPT_2 (busca local ligada)
APENAS_LIST=(true)             # FL_OPT_2_APENAS_MELHOR (elite-only vs pop toda)
# =============================================================================

mkdir -p "$OUTDIR"
BUILD="$OUTDIR/build"
CSV="$OUTDIR/grid_results.csv"
LOG="$OUTDIR/grid.log"
DONE="$OUTDIR/done.keys"
BEST="$OUTDIR/best_so_far.txt"

log() { echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG"; }

# build isolado (copia os fontes uma vez)
rm -rf "$BUILD"; mkdir -p "$BUILD"
cp "$SRC"/*.cpp "$SRC"/*.hpp "$BUILD"/ 2>/dev/null
for inst in "${INSTANCIAS[@]}"; do cp "$INSTDIR/$inst.evrp" "$BUILD"/ 2>/dev/null; done

# cabecalho do CSV (so na 1a vez)
if [[ ! -f "$CSV" ]]; then
  echo "key,timestamp,pop,ktorn,pcross,pmut,estag,fl_opt2,apenas_melhor,instancia,otimo,trials,budget_mult,n_ok,media,desvio,melhor,pior,gap_pct,valores" > "$CSV"
fi
touch "$DONE"

# conta o total de combinacoes p/ mostrar progresso
TOTAL=$(( ${#POP_LIST[@]} * ${#KTORN_LIST[@]} * ${#PCROSS_LIST[@]} * ${#PMUT_LIST[@]} * ${#ESTAG_LIST[@]} * ${#OPT2_LIST[@]} * ${#APENAS_LIST[@]} ))
log "Iniciando grid: $TOTAL combinacoes x ${#INSTANCIAS[@]} instancias x $TRIALS trials (orcamento ${EVAL_MULT}x)."

cfg_i=0
for pop in "${POP_LIST[@]}"; do
for ktorn in "${KTORN_LIST[@]}"; do
for pcross in "${PCROSS_LIST[@]}"; do
for pmut in "${PMUT_LIST[@]}"; do
for estag in "${ESTAG_LIST[@]}"; do
for opt2 in "${OPT2_LIST[@]}"; do
for apenas in "${APENAS_LIST[@]}"; do
  cfg_i=$((cfg_i+1))
  cfg_id="pop${pop}_k${ktorn}_pc${pcross}_pm${pmut}_est${estag}_o2${opt2}_am${apenas}"

  # aplica os parametros nos headers do build (nao toca nos originais)
  sed -i -E "s/#define POP_SIZE .*/#define POP_SIZE ${pop}/;
             s/#define K_TORNEIO .*/#define K_TORNEIO ${ktorn}/;
             s/#define PROB_CROSSOVER .*/#define PROB_CROSSOVER ${pcross}/;
             s/#define PROB_MUTACAO .*/#define PROB_MUTACAO ${pmut}/;
             s/#define LIMITE_ESTAGNACAO .*/#define LIMITE_ESTAGNACAO ${estag}/;
             s/^#define FL_OPT_2 .*/#define FL_OPT_2 ${opt2}/;
             s/#define FL_OPT_2_APENAS_MELHOR .*/#define FL_OPT_2_APENAS_MELHOR ${apenas}/" "$BUILD/heuristic.hpp"
  sed -i -E "s/#define MAX_TRIALS.*/#define MAX_TRIALS ${TRIALS}/" "$BUILD/stats.hpp"
  sed -i -E "s/#define TERMINATION .*/#define TERMINATION ${EVAL_MULT}*ACTUAL_PROBLEM_SIZE/" "$BUILD/EVRP.hpp"

  # compila esta config uma vez (serve p/ todas as instancias)
  if ! ( cd "$BUILD" && g++ -std=c++17 -O2 -o evrp_grid main.cpp EVRP.cpp heuristic.cpp stats.cpp ) 2>>"$LOG"; then
    log "[$cfg_i/$TOTAL] ERRO de compilacao em $cfg_id -- pulando."
    continue
  fi

  for inst in "${INSTANCIAS[@]}"; do
    key="${cfg_id}|${inst}|t${TRIALS}|b${EVAL_MULT}"
    if grep -qxF "$key" "$DONE"; then
      log "[$cfg_i/$TOTAL] $cfg_id / $inst  (ja feito, pulando)"
      continue
    fi

    otimo=$(grep -oiP 'OPTIMAL_VALUE:\s*\K[0-9.]+' "$BUILD/$inst.evrp")
    otimo=${otimo:-0}

    log "[$cfg_i/$TOTAL] rodando $cfg_id / $inst ..."
    t0=$(date +%s)
    # roda; captura as qualidades por trial da saida padrao
    saida=$( cd "$BUILD" && ./evrp_grid "$inst.evrp" 2>/dev/null )
    rm -rf "$BUILD/resultados"      # descarta arquivos por-run p/ nao lotar disco
    vals=$(echo "$saida" | grep -oP 'quality \K[0-9.]+')
    n_ok=$(echo "$vals" | grep -c .)
    dt=$(( $(date +%s) - t0 ))

    if [[ "$n_ok" -eq 0 ]]; then
      log "   -> sem resultado (crash/timeout) em ${dt}s. Nao marca como feito."
      continue
    fi

    # estatisticas
    read media desvio melhor pior gap <<<"$(echo "$vals" | awk -v opt="$otimo" '
      {a[NR]=$1; s+=$1; if(mn==""||$1<mn)mn=$1; if($1>mx)mx=$1}
      END{n=NR; m=s/n; for(i=1;i<=n;i++)v+=(a[i]-m)^2; sd=(n>1)?sqrt(v/(n-1)):0;
          g=(opt>0)?(m-opt)/opt*100:0;
          printf "%.3f %.3f %.3f %.3f %.2f", m, sd, mn, mx, g}')"
    vcsv=$(echo "$vals" | paste -sd'|' -)
    ts=$(date '+%Y-%m-%d %H:%M:%S')

    echo "${key},${ts},${pop},${ktorn},${pcross},${pmut},${estag},${opt2},${apenas},${inst},${otimo},${TRIALS},${EVAL_MULT},${n_ok},${media},${desvio},${melhor},${pior},${gap},${vcsv}" >> "$CSV"
    [[ "$n_ok" -eq "$TRIALS" ]] && echo "$key" >> "$DONE"
    log "   -> media=${media} desvio=${desvio} melhor=${melhor} gap=${gap}% (${n_ok}/${TRIALS} trials, ${dt}s)"

    # atualiza o "melhor ate agora" por instancia (top 8 por media)
    {
      echo "== Melhores combinacoes ate $(date '+%H:%M:%S') =="
      for I in "${INSTANCIAS[@]}"; do
        echo "-- $I --"
        awk -F, -v inst="$I" 'NR>1 && $10==inst {printf "%9.3f  gap=%6.2f%%  %s\n", $15, $19, $3"/"$4"/"$5"/"$6"/o2="$8"/am="$9}' "$CSV" \
          | sort -n | head -8
      done
    } > "$BEST"
  done
done; done; done; done; done; done; done

log "GRID CONCLUIDO. Resultados em $CSV ; ranking em $BEST"
echo
echo "===== TOP 5 POR INSTANCIA ====="
for I in "${INSTANCIAS[@]}"; do
  echo "-- $I --"
  awk -F, -v inst="$I" 'NR>1 && $10==inst {printf "%9.3f  gap=%6.2f%%  pop=%s k=%s pc=%s pm=%s o2=%s am=%s\n", $15, $19, $3,$4,$5,$6,$8,$9}' "$CSV" \
    | sort -n | head -5
done