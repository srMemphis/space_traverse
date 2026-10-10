"""PROTOTYPE (throwaway) for the "clang-format style" wayfinder ticket.

Formats every file in src/ with each candidate *.clang-format in this folder
(without touching the sources) and writes report.html: per-candidate churn
stats and a colored diff per file. Tabs are shown as '→' so tab/space changes
are visible.

Run from anywhere:  py space_traverse/prototype-clang-format/run.py
"""
import difflib
import html
import pathlib
import subprocess

HERE = pathlib.Path(__file__).resolve().parent
SRC = HERE.parent / "src"
CLANG_FORMAT = r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"
FEATURED = ["SimuControl.cpp", "Mesh.cpp", "Track.cpp"]


def decode(b):
    try:
        return b.decode("utf-8")
    except UnicodeDecodeError:
        return b.decode("cp1251")


def lines_of(b):
    return decode(b).replace("\r\n", "\n").split("\n")


def show(s):
    return html.escape(s.replace("\t", "→   "))


files = sorted(p for p in SRC.rglob("*") if p.suffix in (".cpp", ".h"))
featured = [p for name in FEATURED for p in files if p.name == name]
ordered = featured + [p for p in files if p not in featured]
originals = {p: p.read_bytes() for p in files}
total_lines = sum(len(lines_of(b)) for b in originals.values())

variants = []
for cfg in sorted(HERE.glob("*.clang-format")):
    per_file = []
    for p in ordered:
        out = subprocess.run(
            [CLANG_FORMAT, f"--style=file:{cfg}", str(p)], capture_output=True, check=True
        ).stdout
        a, b = lines_of(originals[p]), lines_of(out)
        diff = list(difflib.unified_diff(a, b, lineterm="", n=2))[2:]
        added = sum(1 for d in diff if d.startswith("+"))
        removed = sum(1 for d in diff if d.startswith("-"))
        per_file.append((p, added, removed, diff))
    variants.append((cfg, per_file))

parts = []
for vi, (cfg, per_file) in enumerate(variants):
    rem = sum(r for _, _, r, _ in per_file)
    add = sum(a for _, a, _, _ in per_file)
    touched = sum(1 for _, a, r, _ in per_file if a or r)
    rows = "".join(
        f"<tr><td>{'★ ' if p in featured else ''}{html.escape(str(p.relative_to(SRC)))}</td>"
        f"<td class=n>-{r}</td><td class=n>+{a}</td></tr>"
        for p, a, r, _ in per_file
        if a or r
    )
    diffs = []
    for p, a, r, diff in per_file:
        if not diff:
            continue
        body = "\n".join(
            f"<span class={ {'+': 'add', '-': 'del', '@': 'hunk'}.get(d[:1], 'ctx') }>{show(d)}</span>"
            for d in diff
        )
        diffs.append(
            f"<details{' open' if p in featured else ''}><summary>{html.escape(str(p.relative_to(SRC)))}"
            f" <small>-{r} +{a}</small></summary><pre>{body}</pre></details>"
        )
    cfg_text = html.escape(cfg.read_text())
    parts.append(
        f"<section id=v{vi} class=variant{' hidden' if vi else ''}>"
        f"<h2>{html.escape(cfg.stem)}</h2>"
        f"<p class=stat><b>{rem}</b> of {total_lines} original lines changed "
        f"({100 * rem / total_lines:.1f}%), {touched}/{len(files)} files touched</p>"
        f"<pre class=cfg>{cfg_text}</pre>"
        f"<details><summary>Per-file churn</summary><table>{rows}</table></details>"
        f"<h3>Diffs (★ featured files open)</h3>{''.join(diffs)}</section>"
    )

tabs = "".join(
    f"<button onclick=\"pick({i})\" id=b{i}>{html.escape(c.stem)}</button>" for i, (c, _) in enumerate(variants)
)
page = f"""<!doctype html><meta charset=utf-8><title>clang-format candidates</title>
<style>
:root{{--bg:#fff;--fg:#1d1d1f;--mut:#6b6b70;--add:#e6ffec;--del:#ffebe9;--hunk:#ddf4ff;--line:#ddd}}
@media (prefers-color-scheme:dark){{:root{{--bg:#16171a;--fg:#e8e8ea;--mut:#9a9aa2;--add:#12361f;--del:#3d1518;--hunk:#0f2b46;--line:#333}}}}
body{{background:var(--bg);color:var(--fg);font:14px system-ui;margin:0 auto;max-width:1200px;padding:16px}}
.bar{{position:sticky;top:0;background:var(--bg);padding:8px 0;border-bottom:1px solid var(--line);display:flex;gap:8px;flex-wrap:wrap}}
button{{font:inherit;padding:6px 12px;border:1px solid var(--line);background:none;color:inherit;border-radius:6px;cursor:pointer}}
button.on{{background:var(--fg);color:var(--bg)}}
.hidden{{display:none}} .stat{{font-size:16px}} small,.n{{color:var(--mut)}}
pre{{font:12px/1.4 Consolas,monospace;overflow-x:auto;border:1px solid var(--line);border-radius:6px;padding:8px}}
pre span{{display:block;white-space:pre}} .add{{background:var(--add)}} .del{{background:var(--del)}} .hunk{{background:var(--hunk)}}
summary{{cursor:pointer;margin:6px 0}} td{{padding:1px 12px 1px 0}}
</style>
<p><b>PROTOTYPE</b>: throwaway comparison for the clang-format ticket. Tabs render as <code>→</code>.</p>
<div class=bar>{tabs}</div>{''.join(parts)}
<script>function pick(i){{document.querySelectorAll('.variant').forEach((s,j)=>s.classList.toggle('hidden',i!==j));
document.querySelectorAll('.bar button').forEach((b,j)=>b.classList.toggle('on',i===j));}}pick(0)</script>"""
(HERE / "report.html").write_text(page, encoding="utf-8")

for cfg, per_file in variants:
    rem = sum(r for _, _, r, _ in per_file)
    print(f"{cfg.stem:22} {rem:5}/{total_lines} lines changed ({100 * rem / total_lines:.1f}%)")
    for p, a, r, _ in per_file[: len(featured)]:
        print(f"    {p.name:18} -{r} +{a}")
print("report:", HERE / "report.html")
