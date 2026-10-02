"""Export REPORT_DRAFT.md to the C-named report DOCX.

Professional layout: title page (title, course, group, SUT, date), page
numbers in the footer, styled headings, shaded code blocks, grid tables.
"""
from docx import Document
from docx.shared import Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import re

SRC = "/home/zexr/Desktop/sqe_A2/REPORT_DRAFT.md"
OUT = "/home/zexr/Desktop/sqe_A2/24i3102_24i3052_24i3072_C_Report.docx"

ACCENT = RGBColor(0x1F, 0x4E, 0x79)
MUTED = RGBColor(0x44, 0x44, 0x44)

doc = Document()
style = doc.styles["Normal"]
style.font.name = "Calibri"
style.font.size = Pt(11)

# Title page (from the md title + cover block, then skipped in body parse)
lines = open(SRC).read().splitlines()
title_lines = [l for l in lines if l.startswith("# ")]
title = title_lines[0][2:] if title_lines else "Structural Testing Report"

tp = doc.add_paragraph()
tp.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = tp.add_run("Software Quality Engineering (SE3002)\nAssignment 02 — 100 marks")
r.font.size = Pt(13)
r.font.color.rgb = MUTED
t = doc.add_paragraph()
t.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = t.add_run(title)
r.font.size = Pt(22)
r.bold = True
r.font.color.rgb = ACCENT
sub = doc.add_paragraph()
sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = sub.add_run("PX4-Autopilot v1.17.0  ·  d6f12ad1c4f70ad3230afd7d86e971421e02fef4")
r.font.size = Pt(11)
r.font.color.rgb = MUTED
grp = doc.add_paragraph()
grp.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = grp.add_run("Ali Haider Bajwa (24i3102)  ·  Taimoor Khalid (24i3052)  ·  Ashar Ahmed (24i3072)\nSection C (SE)")
r.font.size = Pt(12)
doc.add_page_break()

# Footer with page numbers (skip title page via different-first-page)
section = doc.sections[0]
section.different_first_page_header_footer = True
footer = section.footer
footer.is_linked_to_previous = False
fp = footer.paragraphs[0]
fp.alignment = WD_ALIGN_PARAGRAPH.CENTER
for tag in ("PAGE", "NUMPAGES"):
    run = fp.add_run()
    f1 = OxmlElement("w:fldChar")
    f1.set(qn("w:fldCharType"), "begin")
    f2 = OxmlElement("w:instrText")
    f2.set(qn("xml:space"), "preserve")
    f2.text = tag
    f3 = OxmlElement("w:fldChar")
    f3.set(qn("w:fldCharType"), "end")
    run._r.append(f1)
    run._r.append(f2)
    run._r.append(f3)
    if tag == "PAGE":
        fp.add_run(" / ")
for run in fp.runs:
    run.font.size = Pt(9)
    run.font.color.rgb = MUTED


def style_heading(p, level):
    for run in p.runs:
        run.font.color.rgb = ACCENT


in_code = False
code_buf = []
table_buf = []
skip_cover = True  # cover block already on the title page


def flush_code():
    global code_buf
    if code_buf:
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(6)
        r = p.add_run("\n".join(code_buf))
        r.font.name = "Consolas"
        r.font.size = Pt(8.5)
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
    if skip_cover:
        if s.startswith("## 1."):
            skip_cover = False
        else:
            continue
    if s.startswith("### "):
        p = doc.add_heading(s[4:], level=3)
        style_heading(p, 3)
    elif s.startswith("## "):
        p = doc.add_heading(s[3:], level=2)
        style_heading(p, 2)
    elif s.startswith("# "):
        continue  # title already on title page
    elif s.startswith("> "):
        p = doc.add_paragraph()
        r = p.add_run(s[2:])
        r.italic = True
        r.font.color.rgb = MUTED
    elif s.startswith("- "):
        p = doc.add_paragraph(style="List Bullet")
        add_rich(p, s[2:])
    elif s.startswith(("*Template", "*Final draft")):
        p = doc.add_paragraph()
        r = p.add_run(s.strip("*"))
        r.italic = True
        r.font.color.rgb = MUTED
    elif s == "---":
        p = doc.add_paragraph()
        r = p.add_run("—" * 30)
        r.font.color.rgb = MUTED
    elif s:
        p = doc.add_paragraph()
        add_rich(p, s)
flush_table()
doc.save(OUT)
print("saved", OUT)
