
import csv, sys
import matplotlib.pyplot as plt

def load(path):
    rows = []
    with open(path, newline='') as f:
        for r in csv.DictReader(f):
            rows.append({"t": int(r["t"]), "from": int(r["from"]), "to": int(r["to"]), "distance": int(r["distance"])})
    return rows

def plot(rows, out="disk_plot.png"):
    if not rows:
        print("Sin datos."); return
    times = [r["t"] for r in rows]
    tos = [r["to"] for r in rows]
    plt.figure()
    plt.step(times, tos, where="post")
    plt.xlabel("t")
    plt.ylabel("cylinder")
    plt.title("Disk head movement")
    plt.tight_layout()
    plt.savefig(out, dpi=160)
    print("Guardado", out)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python plot_disk.py dump.csv [out.png]")
        sys.exit(1)
    rows = load(sys.argv[1])
    out = sys.argv[2] if len(sys.argv) > 2 else "disk_plot.png"
    plot(rows, out)
