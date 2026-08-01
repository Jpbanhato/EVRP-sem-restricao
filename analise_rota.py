#!/usr/bin/env python3
"""
Visualiza a evolucao das rotas do GA (EVRP fase 1).

Uso:
    # modo run unica (rota ou heatmap) -> passa UM log:
    python3 visualizar_rota.py <arquivo.evrp> <log.txt> [--modo rota|heatmap]

    # modo agregado -> passa a PASTA com os logs evo_caminho_run_*.txt:
    python3 visualizar_rota.py <arquivo.evrp> <pasta/> --agregar [--modo rota|heatmap]

Saidas (GIF/PNG) sao gravadas NA PASTA DO ARQUIVO .evrp.

Modos:
    rota     : convergencia da melhor rota (espaguete -> circuito limpo).
    heatmap  : intensidade das arestas (sobe quando mantida, cai forte quando abandonada).

--agregar : usa TODAS as execucoes (arquivos evo_caminho_run_*.txt da pasta).
            rota    -> sobrepoe a rota final de cada run (dispersao).
            heatmap -> frequencia de cada aresta entre as runs (consenso).
"""
import sys, os, glob, math, argparse
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
from PIL import Image
import io

COR = "#386B5E"
COR_DEP = "#C0603A"
RGB = (0.219, 0.419, 0.368)   # COR em 0..1 para alpha por aresta
PISO = 0.10                   # opacidade minima de aresta abandonada
DECAY = 0.45                  # decaimento por geracao (mais forte)

def ler_coordenadas(path):
    coord = []; dentro = False
    for linha in open(path):
        s = linha.strip()
        if s.startswith("NODE_COORD_SECTION"): dentro = True; continue
        if dentro:
            if not s or not s[0].isdigit(): break
            p = s.split(); coord.append((float(p[1]), float(p[2])))
    return coord

def ler_um_log(path):
    """Uma execucao: lista de (ger, len, seq)."""
    run = []; ger_ant = None
    for linha in open(path):
        s = linha.strip()
        if not s.startswith("ger="): continue
        ger = com = None; seq = []
        for campo in s.split(";"):
            if campo.startswith("ger="):  ger = int(campo[4:])
            elif campo.startswith("len="): com = float(campo[4:])
            elif campo.startswith("seq="): seq = [int(x) for x in campo[4:].split("-") if x != ""]
        if seq: run.append((ger, com, seq))
    return run

def ler_pasta(pasta):
    """Todas as execucoes: um arquivo evo_caminho_run_*.txt por run."""
    arqs = sorted(glob.glob(os.path.join(pasta, "evo_caminho_run_*.txt")),
                  key=lambda p: int(''.join(c for c in os.path.basename(p) if c.isdigit()) or 0))
    return [ler_um_log(a) for a in arqs if ler_um_log(a)]

def arestas(seq):
    return [tuple(sorted((seq[i], seq[i+1]))) for i in range(len(seq)-1)]

def comprimento(coord, seq):
    return sum(math.dist(coord[seq[i]], coord[seq[i+1]]) for i in range(len(seq)-1))

def com_ou_calc(coord, com, seq):
    return com if com is not None else comprimento(coord, seq)

def limites(coord):
    xs=[c[0] for c in coord]; ys=[c[1] for c in coord]
    mx=(max(xs)-min(xs))*0.08; my=(max(ys)-min(ys))*0.08
    return (min(xs)-mx,max(xs)+mx),(min(ys)-my,max(ys)+my)

def base_ax(xlim,ylim,com_curva=False):
    if com_curva:
        fig,(ax,axc)=plt.subplots(1,2,figsize=(16,10),dpi=100,
                                  gridspec_kw={"width_ratios":[3,3],"wspace":0.05})
        ax.set_xlim(xlim); ax.set_ylim(ylim); ax.set_aspect("equal"); ax.axis("off")
        return fig,ax,axc
    fig,ax=plt.subplots(figsize=(6,6),dpi=100)
    ax.set_xlim(xlim); ax.set_ylim(ylim); ax.set_aspect("equal"); ax.axis("off")
    return fig,ax

def desenhar_nos(ax, coord, ativos):
    for v,(x,y) in enumerate(coord):
        if v not in ativos:
            ax.scatter([x],[y], s=35, color=COR, alpha=0.25, zorder=2)
    for v in ativos:
        if v==0: continue
        x,y=coord[v]; ax.scatter([x],[y], s=45, color=COR, alpha=1.0, zorder=3)
    dx,dy=coord[0]
    ax.scatter([dx],[dy], s=150, marker="s", color=COR_DEP,
               edgecolors="white", linewidths=1.2, zorder=4, label="depósito")

def titulo(ax, ger=None, com=None, extra=None):
    if extra is not None:
        t = extra
    else:
        t = f"geração {ger}"
        if com is not None: t += f"   •   comprimento = {com:.2f}"
    ax.set_title(t, fontsize=13, color=COR)

def desenhar_curva(axc, curva, ger_atual):
    """Miniatura lateral da convergencia (geracao x comprimento) com ponto vermelho na geracao atual."""
    gs = [g for (g, _) in curva]; ls = [l for (_, l) in curva]
    axc.plot(gs, ls, color=COR, lw=1.4)
    la = dict(curva).get(ger_atual)
    if la is None:
        j = min(range(len(gs)), key=lambda i: abs(gs[i]-ger_atual)); la = ls[j]; gx = gs[j]
    else:
        gx = ger_atual
    axc.scatter([gx],[la], color="#D33A2C", s=34, zorder=5)
    axc.set_title("convergência", fontsize=10, color=COR, pad=4)
    axc.set_xlabel("geração", fontsize=8, labelpad=2)
    axc.set_ylabel("comprimento", fontsize=8, labelpad=2)
    axc.tick_params(labelsize=7, length=2)
    for sp in axc.spines.values(): sp.set_alpha(0.4)
    axc.grid(True, alpha=0.25, linestyle="--", linewidth=0.5)

def fig_img(fig):
    buf=io.BytesIO(); fig.savefig(buf,format="png"); plt.close(fig); buf.seek(0)
    return Image.open(buf).convert("RGB")

def salvar_gif(imgs, path, fim=2500):
    dur=[350]*len(imgs); dur[-1]=fim
    imgs[0].save(path, save_all=True, append_images=imgs[1:], duration=dur, loop=0)

def desenhar_arestas(ax, coord, intens):
    segs=[]; cols=[]
    for (a,b),w in intens.items():
        if w<0.02: continue
        segs.append([coord[a],coord[b]]); cols.append((*RGB, min(1.0,w)))
    if segs: ax.add_collection(LineCollection(segs, colors=cols, linewidths=1.7, zorder=1))

# ---------- ROTA ----------
def quadro_rota(coord, ger, com, seq, xlim, ylim, curva=None):
    if curva is not None: fig,ax,axc=base_ax(xlim,ylim,com_curva=True)
    else:                 fig,ax=base_ax(xlim,ylim)
    xs=[coord[v][0] for v in seq]; ys=[coord[v][1] for v in seq]
    ax.plot(xs,ys,"-",color=COR,lw=1.6,zorder=1)
    desenhar_nos(ax,coord,set(seq)); titulo(ax,ger,com)
    ax.legend(loc="upper right",frameon=False,fontsize=9)
    if curva is not None: desenhar_curva(axc, curva, ger)
    fig.tight_layout()
    return fig_img(fig)

def modo_rota(coord, runs, agregar, base, xlim, ylim, curva_flag=False):
    if not agregar:
        run=min(runs,key=lambda r: com_ou_calc(coord, r[-1][1], r[-1][2]))
        curva=[(g, com_ou_calc(coord,c,seq)) for (g,c,seq) in run]
        imgs=[quadro_rota(coord,g,com_ou_calc(coord,c,seq),seq,xlim,ylim,
                          curva if curva_flag else None)
              for (g,c,seq) in run]
        salvar_gif(imgs, base+"_rota.gif"); imgs[-1].save(base+"_rota_final.png")
        print("gravado:", base+"_rota.gif", "+", base+"_rota_final.png")
    else:
        fig,ax=base_ax(xlim,ylim); ativos=set(); melhores=[]
        for run in runs:
            seq=run[-1][2]; melhores.append(com_ou_calc(coord,run[-1][1],seq)); ativos|=set(seq)
            xs=[coord[v][0] for v in seq]; ys=[coord[v][1] for v in seq]
            ax.plot(xs,ys,"-",color=COR,lw=1.0,alpha=0.28,zorder=1)
        desenhar_nos(ax,coord,ativos)
        titulo(ax,extra=f"rota final de {len(runs)} execuções   •   melhor = {min(melhores):.2f}")
        fig.tight_layout(); fig_img(fig).save(base+"_rota_todas.png")
        print("gravado:", base+"_rota_todas.png")

# ---------- HEATMAP ----------
def modo_heatmap(coord, runs, agregar, base, xlim, ylim, curva_flag=False):
    if not agregar:
        run=min(runs,key=lambda r: com_ou_calc(coord, r[-1][1], r[-1][2]))
        curva=[(g, com_ou_calc(coord,c,seq)) for (g,c,seq) in run]
        intens={}; imgs=[]
        for (g,c,seq) in run:
            atuais=set(arestas(seq))
            for e in list(intens):
                if e not in atuais: intens[e]=max(PISO*0.5, intens[e]*DECAY)  # decai a cada geracao ausente
            for e in atuais: intens[e]=1.0                                     # reforca as ativas
            if curva_flag: fig,ax,axc=base_ax(xlim,ylim,com_curva=True)
            else:          fig,ax=base_ax(xlim,ylim)
            desenhar_arestas(ax,coord,intens)
            desenhar_nos(ax,coord,set(seq)); titulo(ax,g,com_ou_calc(coord,c,seq))
            if curva_flag: desenhar_curva(axc, curva, g)
            fig.tight_layout()
            imgs.append(fig_img(fig))
        salvar_gif(imgs, base+"_heatmap.gif"); imgs[-1].save(base+"_heatmap_final.png")
        print("gravado:", base+"_heatmap.gif", "+", base+"_heatmap_final.png")
    else:
        freq={}; ativos=set(); melhores=[]
        for run in runs:
            seq=run[-1][2]; melhores.append(com_ou_calc(coord,run[-1][1],seq)); ativos|=set(seq)
            for e in set(arestas(seq)): freq[e]=freq.get(e,0)+1
        n=len(runs); intens={e:max(PISO,c/n) for e,c in freq.items()}
        fig,ax=base_ax(xlim,ylim); desenhar_arestas(ax,coord,intens)
        desenhar_nos(ax,coord,ativos)
        titulo(ax,extra=f"frequência das arestas em {n} execuções   •   melhor = {min(melhores):.2f}")
        fig.tight_layout(); fig_img(fig).save(base+"_heatmap_freq.png")
        print("gravado:", base+"_heatmap_freq.png")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("evrp")
    ap.add_argument("entrada", help="log de UMA run, ou PASTA com evo_caminho_run_*.txt se --agregar")
    ap.add_argument("--modo", choices=["rota","heatmap"], default="rota")
    ap.add_argument("--agregar", action="store_true")
    ap.add_argument("--curva", action="store_true", help="adiciona miniatura da convergencia com ponto na geracao atual")
    a=ap.parse_args()

    coord=ler_coordenadas(a.evrp)
    if a.agregar:
        pasta = a.entrada if os.path.isdir(a.entrada) else os.path.dirname(a.entrada) or "."
        runs=ler_pasta(pasta)
        if not runs: print("Nenhum evo_caminho_run_*.txt encontrado em", pasta); sys.exit(1)
    else:
        runs=[ler_um_log(a.entrada)]
        if not runs[0]: print("Log vazio."); sys.exit(1)

    xlim,ylim=limites(coord)
    # saida na pasta do LOG (arquivo de run) ou na propria pasta passada em --agregar
    if a.agregar:
        pasta_saida = a.entrada if os.path.isdir(a.entrada) else (os.path.dirname(a.entrada) or ".")
    else:
        pasta_saida = os.path.dirname(os.path.abspath(a.entrada)) or "."
    prefixo=os.path.splitext(os.path.basename(a.evrp))[0]
    base=os.path.join(pasta_saida, prefixo)

    if a.modo=="rota": modo_rota(coord,runs,a.agregar,base,xlim,ylim,a.curva)
    else:              modo_heatmap(coord,runs,a.agregar,base,xlim,ylim,a.curva)

if __name__=="__main__":
    main()