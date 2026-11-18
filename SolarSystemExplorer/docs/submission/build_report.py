"""Build the course report PDF from REPORT.md and evidence.json.

Requires ReportLab. Run with Python 3 and pass the desired PDF output path.
Screenshot paths in evidence.json are relative to the repository root.
"""

import argparse
from html import escape
import json
import os
from pathlib import Path
import re

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    Flowable, Image, PageBreak, Paragraph, SimpleDocTemplate, Spacer, Table, TableStyle
)


SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[1]
INK = colors.HexColor("#172333")
ACCENT = colors.HexColor("#174C61")
MUTED = colors.HexColor("#48606F")
BORDER = colors.HexColor("#CFDBE1")
TINT = colors.HexColor("#EDF4F6")
PAGE_WIDTH = A4[0] - 100

FONT_DIR = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts"
for name, filename in {
    "Report": "arial.ttf",
    "Report-Bold": "arialbd.ttf",
    "Report-Italic": "ariali.ttf",
    "Report-BoldItalic": "arialbi.ttf",
    "Equation": "consola.ttf",
}.items():
    pdfmetrics.registerFont(TTFont(name, FONT_DIR / filename))
pdfmetrics.registerFontFamily(
    "Report", normal="Report", bold="Report-Bold",
    italic="Report-Italic", boldItalic="Report-BoldItalic",
)

STYLES = {
    "title": ParagraphStyle("Title", fontName="Report-Bold", fontSize=28, leading=33, spaceAfter=12),
    "heading": ParagraphStyle("Heading", fontName="Report-Bold", fontSize=15, leading=18,
                              textColor=ACCENT, spaceBefore=11, spaceAfter=8, keepWithNext=True),
    "body": ParagraphStyle("Body", fontName="Report", fontSize=10.2, leading=14,
                           textColor=INK, spaceAfter=7, splitLongWords=False),
    "table": ParagraphStyle("Table", fontName="Report", fontSize=9.2, leading=12, textColor=INK),
    "table_head": ParagraphStyle("Table header", fontName="Report-Bold", fontSize=9.2, leading=12,
                                 textColor=ACCENT),
    "caption": ParagraphStyle("Caption", fontName="Report-Italic", fontSize=8.7, leading=11,
                              textColor=MUTED, spaceBefore=3, spaceAfter=6),
    "reference": ParagraphStyle("Reference", fontName="Report", fontSize=9.2, leading=12,
                                textColor=INK, spaceAfter=7, leftIndent=13, firstLineIndent=-13),
}


def markup(value):
    value = escape(value)
    value = re.sub(r"https?://[^\s<]+", lambda match: f'<link href="{match[0]}">{match[0]}</link>', value)
    value = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", value)
    value = re.sub(r"\*(.+?)\*", r"<i>\1</i>", value)
    value = re.sub(r"`(.+?)`", r'<font name="Equation" size="9">\1</font>', value)
    return value.replace("\n", "<br/>")


def para(text, style="body"):
    return Paragraph(markup(text), STYLES[style])


class Pipeline(Flowable):
    def __init__(self):
        super().__init__()
        self.width = PAGE_WIDTH
        self.height = 166

    def draw(self):
        canvas = self.canv
        gap = 18
        node_width = (self.width - gap * 2) / 3

        def node(col, y, label, detail):
            x = col * (node_width + gap)
            canvas.setFillColor(TINT)
            canvas.setStrokeColor(BORDER)
            canvas.roundRect(x, y, node_width, 39, 5, stroke=1, fill=1)
            canvas.setFont("Report-Bold", 9.4)
            canvas.setFillColor(INK)
            canvas.drawCentredString(x + node_width / 2, y + 23, label)
            canvas.setFont("Report", 8.1)
            canvas.setFillColor(MUTED)
            canvas.drawCentredString(x + node_width / 2, y + 10, detail)

        def arrow(x1, y1, x2, y2):
            canvas.setStrokeColor(ACCENT)
            canvas.setFillColor(ACCENT)
            canvas.line(x1, y1, x2, y2)
            tip = canvas.beginPath()
            tip.moveTo(x2, y2)
            if y1 == y2:
                offset = -4 if x2 > x1 else 4
                tip.lineTo(x2 + offset, y2 + 2.5)
                tip.lineTo(x2 + offset, y2 - 2.5)
            else:
                tip.lineTo(x2 - 2.5, y2 + 4)
                tip.lineTo(x2 + 2.5, y2 + 4)
            tip.close()
            canvas.drawPath(tip, stroke=0, fill=1)

        node(0, 119, "Geometry and materials", "Vertex and fragment shaders")
        node(1, 119, "HDR scene target", "Linear color, depth and stencil")
        node(2, 119, "Resolve multisampling", "Single-sample scene texture")
        node(2, 64, "Extract highlights", "Component-wise excess over 1")
        node(1, 64, "Separable blur", "Horizontal and vertical passes")
        node(0, 64, "Composite", "Resolved scene plus optional glow")
        node(0, 9, "Display transform", "Exposure, tone map and gamma")
        node(1, 9, "Window image", "Fullscreen output")
        node(2, 9, "Interface", "ImGui overlay")
        for y, direction in [(138, 1), (83, -1), (28, 1)]:
            for col in range(2):
                x1 = col * (node_width + gap) + node_width + 2
                x2 = x1 + gap - 4
                left, right = (x1, x2) if direction > 0 else (x2, x1)
                arrow(left, y, right, y)
        arrow(node_width / 2, 62, node_width / 2, 50)
        resolve_x = self.width - node_width / 2
        arrow(resolve_x, 117, resolve_x, 105)
        # The resolved scene also goes directly to the composite.
        canvas.setStrokeColor(ACCENT)
        canvas.line(resolve_x - 20, 117, resolve_x - 20, 110)
        canvas.line(resolve_x - 20, 110, node_width / 2, 110)
        arrow(node_width / 2, 110, node_width / 2, 105)


def table(lines):
    rows = [[cell.strip() for cell in line.strip().strip("|").split("|")] for line in lines]
    rows.pop(1)
    fractions = [.36, .64] if len(rows[0]) == 2 else [.22, .20, .25, .33]
    data = [[para(cell, "table_head" if index == 0 else "table") for cell in row]
            for index, row in enumerate(rows)]
    result = Table(data, colWidths=[PAGE_WIDTH * value for value in fractions], repeatRows=1,
                   hAlign="LEFT", spaceAfter=9)
    result.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), TINT),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#F7FAFB")]),
        ("LINEBELOW", (0, 0), (-1, 0), .6, BORDER),
        ("LINEBELOW", (0, -1), (-1, -1), .6, BORDER),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("TOPPADDING", (0, 0), (-1, -1), 5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
        ("LEFTPADDING", (0, 0), (-1, -1), 8),
        ("RIGHTPADDING", (0, 0), (-1, -1), 8),
    ]))
    return result


def photo(key, evidence):
    if key not in evidence:
        return Spacer(1, 2)
    path = ROOT / evidence[key]
    width, height = ImageReader(path).getSize()
    scale = min(PAGE_WIDTH / width, 286 / height)
    return Image(str(path), width=width * scale, height=height * scale, hAlign="LEFT")


def content(evidence):
    template = (SOURCE / "REPORT.md").read_text(encoding="utf-8-sig")
    template = re.sub(r"\{\{([A-Z_]+)\}\}", lambda match: evidence[match[1]], template)
    sections = re.split(r"\n\s*\n", template.strip())
    references = False
    for section in sections:
        if section.startswith("# "):
            yield para(section[2:], "title")
        elif section.startswith("## "):
            references = section == "## References"
            yield para(section[3:], "heading")
        elif section.startswith("|"):
            yield table(section.splitlines())
        elif section == "::: page":
            yield PageBreak()
        elif section == "::: pipeline":
            yield Pipeline()
        elif section == "::: evidence":
            yield PageBreak()
            if "bloom" in evidence or "workshop" in evidence:
                yield para("Screenshots from the Release build", "heading")
                for key in ("bloom", "workshop"):
                    if key in evidence:
                        yield photo(key, evidence)
                        yield para(evidence[f"{key.upper()}_CAPTION"], "caption")
                yield PageBreak()
        elif section.startswith("Figure "):
            yield para(section, "caption")
        else:
            section = re.sub(r"(?<! )\n", " ", section).replace("  \n", "\n")
            yield para(section, "reference" if references else "body")


def footer(canvas, doc):
    canvas.setFillColor(MUTED)
    canvas.setFont("Report", 8)
    canvas.drawString(50, 27, "Solar System Explorer")
    canvas.drawRightString(A4[0] - 50, 27, str(doc.page))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    evidence = json.loads((SOURCE / "evidence.json").read_text(encoding="utf-8-sig"))
    report = SimpleDocTemplate(
        str(args.output), pagesize=A4, leftMargin=50, rightMargin=50,
        topMargin=45, bottomMargin=44, title="Solar System Explorer",
        author="Abai Erik, Nurbolat Zhumadulla, Nurasyl Adildabek, Ulnaz Seitzhan",
    )
    report.build(list(content(evidence)), onFirstPage=footer, onLaterPages=footer)


if __name__ == "__main__":
    main()
