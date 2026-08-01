import re
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

# arquivo = "E-n22-k4.evrp"
arquivo = "E-n76-k7.evrp"

resultados_caminho = Path(f"resultados/{arquivo}")

if not resultados_caminho.exists():
    print(f"Pasta '{resultados_caminho}' não encontrada")
    exit()

arquivos = sorted(resultados_caminho.glob("*.txt"))

if not arquivos:
    print("Nenhum arquivo TXT encontrado na pasta 'resultados'")
    exit()

# Lista para guardar os DataFrames de todas as runs
todos_dados = []

for arquivo in arquivos:
    dados_arquivo = []
    
    # Lendo e limpando o formato "pop=0,best=646.497"
    with open(arquivo, "r") as f:
        for linha in f:
            # Procura por números logo após "pop=" e "best="
            match = re.search(r"pop=(\d+),best=([\d.]+)", linha.strip())
            if match:
                pop_val = int(match.group(1))
                best_val = float(match.group(2))
                dados_arquivo.append({"pop": pop_val, "best": best_val})
                
    if dados_arquivo:
        df_run = pd.DataFrame(dados_arquivo)
        todos_dados.append(df_run)

# Junta todos os dados em um único DataFrame gigante
df_completo = pd.concat(todos_dados, ignore_index=True)

# Agrupa por geração calculando a Média e o Desvio Padrão entre as runs
df_resumo = df_completo.groupby("pop")["best"].agg(["mean", "std"]).reset_index()

# Se houver apenas 1 run, o desvio padrão será NaN. Vamos preencher com 0 nesse caso.
df_resumo["std"] = df_resumo["std"].fillna(0)

# --- Plotagem do Gráfico ---
plt.figure(figsize=(12, 6))

# 1. Linha principal: Média das runs
plt.plot(df_resumo["pop"], df_resumo["mean"], color="#1f77b4", label="Média das Runs", linewidth=2.5, marker='o', markersize=4)

# 2. Sombra: Variação (Média - Desvio Padrão até Média + Desvio Padrão)
plt.fill_between(
    df_resumo["pop"],
    df_resumo["mean"] - df_resumo["std"],
    df_resumo["mean"] + df_resumo["std"],
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

# Correção do salvamento: combinando caminhos com a Pathlib da forma correta
caminho_salvamento = resultados_caminho / "resultados_analise.png"
plt.savefig(caminho_salvamento, dpi=300)

print(f"Gráfico plotado com sucesso e salvo em: {caminho_salvamento}")
plt.show()