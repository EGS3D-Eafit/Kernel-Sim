
import sys
import os
import pandas as pd
import matplotlib.pyplot as plt


def main():
    if len(sys.argv) != 2:
        print("Uso: python fallos_vs_marcos.py <archivo.csv>")
        print("El CSV debe tener las columnas: frames,faults")
        sys.exit(1)

    csv_path = sys.argv[1]
    if not os.path.isfile(csv_path):
        print(f"Archivo no encontrado: {csv_path}")
        sys.exit(1)

    try:
        df = pd.read_csv(csv_path)
    except Exception as e:
        print(f"No se pudo leer el CSV: {e}")
        sys.exit(1)

    # Intento de normalización de nombres de columnas
    cols = {c.lower().strip(): c for c in df.columns}
    # Buscar sinónimos básicos
    frames_col = None
    faults_col = None
    for c in df.columns:
        cl = c.lower().strip()
        if cl in ("frames", "marcos", "num_frames", "n_frames"):
            frames_col = c if frames_col is None else frames_col
        if cl in ("faults", "fallos", "page_faults", "pf", "faltas"):
            faults_col = c if faults_col is None else faults_col

    if frames_col is None or faults_col is None:
        print("CSV inválido: se requieren columnas 'frames' y 'faults' (o sinónimos: marcos/fallos).")
        print(f"Columnas detectadas: {list(df.columns)}")
        sys.exit(1)

    # Ordenar por frames por si no viene ordenado
    df = df[[frames_col, faults_col]].copy()
    df = df.sort_values(by=frames_col)

    # Plot único (sin estilos/colores específicos)
    plt.figure()
    plt.plot(df[frames_col].values, df[faults_col].values, marker='o')
    plt.xlabel("Tamaño de marcos (frames)")
    plt.ylabel("Fallos de página")
    plt.title("Fallos vs Tamaño de Marcos")
    plt.grid(True)

    # Guardar al lado del CSV
    out_png = os.path.splitext(csv_path)[0] + "_fallos_vs_marcos.png"
    try:
        plt.savefig(out_png, bbox_inches="tight", dpi=120)
        print(f"Gráfica guardada en: {out_png}")
    except Exception as e:
        print(f"No se pudo guardar la gráfica: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
