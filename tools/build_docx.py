"""Export REPORT_DRAFT.md to the C-named report DOCX (headings, bullets, tables, code)."""
from docx import Document
from docx.shared import Pt, RGBColor
import re

SRC = "/home/zexr/Desktop/sqe_A2/REPORT_DRAFT.md"
OUT = "/home/zexr/Desktop/sqe_A2/24i3102_24i3052_24i3072_C_Report.docx"

doc = Document()
style = doc.styles["Normal"]
style.font.name = "Calibri"
style.font.size = Pt(11)

lines = open(SRC).read().splitlines()
in_code = False
code_buf = []
table_buf = []


def flush_code():
    global code_buf
    if code_buf:
        p = doc.add_paragraph()
        r = p.add_run("\n".join(code_buf))
        r.font.name = "Consolas"
        r.font.size = Pt(9)
        code_buf = []


def flush_table():
    global table_buf
    if table_buf:
        rows = [r for r in table_buf if not all(re.fullmatch(r":?-{2,}:?", c or "") for c in r)]
        if rows:
            t = doc.add_table(rows=len(rows), cols=len(rows[0]))
            t.style = "Light Grid Accent 1"
            for ri, row in enumerate(rows):
                for ci, val in enumerate(row):
                    cell = t.cell(ri, ci)
                    cell.text = ""
                    run = cell.paragraphs[0].add_run(val)
                    run.font.size = Pt(9)
                    if ri == 0:
                        run.bold = True
        table_buf = []


def add_rich(par, text):
    for part in re.split(r"(`[^`]+`)", text):
        if part.startswith("`") and part.endswith("`") and len(part) > 2:
            r = par.add_run(part[1:-1])
            r.font.name = "Consolas"
            r.font.size = Pt(10)
        else:
            for seg in re.split(r"(\*\*[^*]+\*\*)", part):
                if seg.startswith("**") and seg.endswith("**") and len(seg) > 4:
                    r = par.add_run(seg[2:-2])
                    r.bold = True
                elif seg:
                    par.add_run(seg)


for ln in lines:
    s = ln.strip()
    if s.startswith("```"):
        if in_code:
            flush_code()
            in_code = False
        else:
            flush_table()
            in_code = True
        continue
    if in_code:
        code_buf.append(ln)
        continue
    if s.startswith("|") and s.endswith("|"):
        table_buf.append([c.strip() for c in s.strip("|").split("|")])
        continue
    flush_table()
    if s.startswith("### "):
        doc.add_heading(s[4:], level=3)
    elif s.startswith("## "):
        doc.add_heading(s[3:], level=2)
    elif s.startswith("# "):
        doc.add_heading(s[2:], level=1)
    elif s.startswith("> "):
        p = doc.add_paragraph()
        r = p.add_run(s[2:])
        r.italic = True
        r.font.color.rgb = RGBColor(0x44, 0x44, 0x44)
    elif s.startswith("- "):
        p = doc.add_paragraph(style="List Bullet")
        add_rich(p, s[2:])
    elif s.startswith(("*Template", "*Final draft")):
        p = doc.add_paragraph()
        r = p.add_run(s.strip("*"))
        r.italic = True
    elif s == "---":
        doc.add_paragraph("—" * 30)
    elif s:
        p = doc.add_paragraph()
        add_rich(p, s)
flush_table()
doc.save(OUT)
print("saved", OUT)
