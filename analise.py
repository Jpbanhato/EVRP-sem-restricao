import re
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

# arquivo = "E-n22-k4.evrp"
arquivo = "E-n76-k7.evrp"

resultados_caminho = Path(f"resultados/{arquivo}")

if not resultados_caminho.exists():
    print(f"Pasta '{resultados_caminho}' não encontrada")
    exit()

# só os logs de run (evita pegar outros .txt que existam na pasta)
arquivos = sorted(resultados_caminho.glob("analise_run_*.txt"))

if not arquivos:
    print("Nenhum arquivo analise_run_*.txt encontrado na pasta")
    exit()

# Lê o best-so-far de cada run (ordenado por geração)
runs = []
for arq in arquivos:
    dados = []
    with open(arq, "r") as f:
        for linha in f:
            match = re.search(r"pop=(\d+),best=([\d.]+)", linha.strip())
            if match:
                dados.append((int(match.group(1)), float(match.group(2))))
    if dados:
        dados.sort(key=lambda t: t[0])          # garante ordem por geração
        runs.append(np.array([b for _, b in dados]))

if not runs:
    print("Nenhum dado lido")
    exit()

# --- CORREÇÃO DO ARTEFATO DE CAUDA ---
# Runs terminam em gerações diferentes (o orçamento é por avaliações, não por gerações).
# Alinhamos todas ao mesmo comprimento, carregando o best final de cada run até o fim
# (forward-fill). Assim toda run contribui em toda geração e a média fica monotônica.
L = max(len(r) for r in runs)
mat = np.empty((len(runs), L))
for i, r in enumerate(runs):
    mat[i, :len(r)] = r
    mat[i, len(r):] = r[-1]      # após terminar, o best da run permanece no valor final

media  = mat.mean(axis=0)
desvio = mat.std(axis=0)
geracoes = np.arange(L)

# --- Plotagem ---
plt.figure(figsize=(12, 6))

plt.plot(geracoes, media, color="#1f77b4", label="Média das Runs", linewidth=2.5)

plt.fill_between(
    geracoes,
    media - desvio,
    media + desvio,
    color="#1f77b4",
    alpha=0.2,
    label="Variação (Desvio Padrão)"
)

plt.xlabel("Geração")
plt.ylabel("Melhor Fitness (best)")
plt.title("Evolução dos Resultados: Média e Variabilidade entre Runs")
plt.legend(loc="upper right")
plt.grid(True, alpha=0.3, linestyle="--")
plt.tight_layout()

caminho_salvamento = resultados_caminho / "resultados_analise.png"
plt.savefig(caminho_salvamento, dpi=300)

print(f"Gráfico salvo em: {caminho_salvamento}  ({len(runs)} runs, {L} gerações)")
plt.show()