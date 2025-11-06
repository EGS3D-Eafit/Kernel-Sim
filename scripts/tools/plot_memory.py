"""
plot_memory.py
Lee CSVs de results/mem/*.csv (exportados por la CLI) y genera gráficas comparativas.
- Faults vs Frames
- Total Time vs Frames
- Miss Rate vs Frames (si hay datos suficientes)

Estructuras de columnas soportadas (se normalizan automáticamente):
- Política:  policy | algo | replacement
- Marcos:    frames | num_frames | nframes
- Fallos:    faults | page_faults | misses
- Accesos:   accesses | refs | references | naccess
- Tiempo:    total_time_ms | time_ms | time | total_ms

Salida:
- figs/mem_faults_vs_frames.png
- figs/mem_time_vs_frames.png
- figs/mem_missrate_vs_frames.png (si hay accesos)
- results/mem/_summary.csv
"""

import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

# ------------------------- Utilidades -------------------------


def ensure_dir(path: str) -> None:
    if not os.path.isdir(path):
        os.makedirs(path, exist_ok=True)


def try_col(df: pd.DataFrame, candidates):
    for c in candidates:
        if c in df.columns:
            return c
    return None


def normalize_columns(df: pd.DataFrame) -> pd.DataFrame:
    # renombra columnas a un set canónico si existen
    mapping = {}
    policy_col = try_col(df, ["policy", "algo", "replacement"])
    frames_col = try_col(df, ["frames", "num_frames", "nframes"])
    faults_col = try_col(df, ["faults", "page_faults", "misses"])
    acc_col = try_col(df, ["accesses", "refs", "references", "naccess"])
    time_col = try_col(df, ["total_time_ms", "time_ms", "time", "total_ms"])

    if policy_col:
        mapping[policy_col] = "policy"
    if frames_col:
        mapping[frames_col] = "frames"
    if faults_col:
        mapping[faults_col] = "faults"
    if acc_col:
        mapping[acc_col] = "accesses"
    if time_col:
        mapping[time_col] = "total_time_ms"

    df = df.rename(columns=mapping)

    # tipado básico
    if "frames" in df.columns:
        df["frames"] = pd.to_numeric(df["frames"], errors="coerce")
    if "faults" in df.columns:
        df["faults"] = pd.to_numeric(df["faults"], errors="coerce")
    if "accesses" in df.columns:
        df["accesses"] = pd.to_numeric(df["accesses"], errors="coerce")
    if "total_time_ms" in df.columns:
        df["total_time_ms"] = pd.to_numeric(
            df["total_time_ms"], errors="coerce")

    # relleno de policy si falta
    if "policy" not in df.columns:
        df["policy"] = "unknown"

    # derivadas
    if "accesses" in df.columns and "faults" in df.columns:
        with pd.option_context('mode.use_inf_as_na', True):
            df["miss_rate"] = (df["faults"] / df["accesses"]
                               ).replace([float("inf")], pd.NA)
    else:
        df["miss_rate"] = pd.NA

    return df


def read_all_results(pattern="results/mem/*.csv") -> pd.DataFrame:
    files = glob.glob(pattern)
    if not files:
        raise SystemExit(f"No se encontraron CSV en: {pattern}\n"
                         "Asegúrate de ejecutar primero los scripts que exportan resultados.")
    frames = []
    for f in files:
        try:
            df = pd.read_csv(f)
            df = normalize_columns(df)
            df["__file__"] = os.path.basename(f)
            frames.append(df)
        except Exception as e:
            print(f"[WARN] No pude leer {f}: {e}")
    if not frames:
        raise SystemExit("No se pudo cargar ningún CSV válido.")
    out = pd.concat(frames, ignore_index=True)
    # Filtros mínimos razonables
    if "frames" in out.columns:
        out = out.dropna(subset=["frames"])
    return out


def plot_lines(df: pd.DataFrame, x: str, y: str, title: str, out_png: str, y_label: str):
    plt.figure()  # gráfico único, sin estilos ni colores específicos
    # agrupamos por política y ordenamos por frames para trazar líneas coherentes
    for policy, sub in df.groupby("policy"):
        # Asegura orden por X
        sub = sub.sort_values(by=x)
        plt.plot(sub[x], sub[y], marker="o", label=str(policy))
    plt.title(title)
    plt.xlabel(x)
    plt.ylabel(y_label)
    plt.legend()
    plt.grid(True, which="both", linestyle="--", linewidth=0.5, alpha=0.7)
    plt.tight_layout()
    plt.savefig(out_png, dpi=160)
    plt.close()
    print(f"[OK] Guardado: {out_png}")

# ------------------------- Main -------------------------


def main():
    # rutas de E/S
    results_dir = "results/mem"
    figs_dir = "figs"
    ensure_dir(results_dir)
    ensure_dir(figs_dir)

    df = read_all_results(os.path.join(results_dir, "*.csv"))

    # Guardar resumen combinado para trazabilidad
    summary_path = os.path.join(results_dir, "_summary.csv")
    cols_order = [c for c in ["policy", "frames", "faults", "accesses",
                              "miss_rate", "total_time_ms", "__file__"] if c in df.columns]
    df[cols_order].to_csv(summary_path, index=False)
    print(f"[OK] Resumen combinado: {summary_path}")

    # Validaciones mínimas
    if "frames" not in df.columns:
        raise SystemExit(
            "No hay columna 'frames' en los CSV normalizados. Verifica tus exportaciones.")

    # --- Plot 1: Faults vs Frames ---
    if "faults" in df.columns:
        plot_lines(
            df=df.dropna(subset=["faults"]),
            x="frames",
            y="faults",
            title="Page Faults vs Frames",
            out_png=os.path.join(figs_dir, "mem_faults_vs_frames.png"),
            y_label="Page Faults"
        )
    else:
        print("[INFO] No hay columna 'faults' para graficar Faults vs Frames.")

    # --- Plot 2: Total Time vs Frames ---
    if "total_time_ms" in df.columns:
        plot_lines(
            df=df.dropna(subset=["total_time_ms"]),
            x="frames",
            y="total_time_ms",
            title="Total Time vs Frames",
            out_png=os.path.join(figs_dir, "mem_time_vs_frames.png"),
            y_label="Total Time (ms)"
        )
    else:
        print("[INFO] No hay columna 'total_time_ms' para graficar Time vs Frames.")

    # --- Plot 3 (opcional): Miss Rate vs Frames ---
    if "miss_rate" in df.columns and df["miss_rate"].notna().any():
        plot_lines(
            df=df.dropna(subset=["miss_rate"]),
            x="frames",
            y="miss_rate",
            title="Miss Rate vs Frames",
            out_png=os.path.join(figs_dir, "mem_missrate_vs_frames.png"),
            y_label="Miss Rate (faults/accesses)"
        )
    else:
        print(
            "[INFO] No fue posible calcular 'miss_rate' (faltan 'accesses' o 'faults').")

    print("\nListo. Revisa la carpeta 'figs/' para las imágenes y 'results/mem/_summary.csv' para el combinado.")


if __name__ == "__main__":
    main()
