"""Build the illustrated English PDF from User Guide.md (ReportLab and Pillow)."""
from html import escape
from pathlib import Path
from urllib.parse import unquote, urljoin
import os
import re

from PIL import Image as PillowImage
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import CondPageBreak, Image, PageBreak, Paragraph, Preformatted, SimpleDocTemplate, Spacer, Table, TableStyle

ROOT = Path(__file__).resolve().parents[1]
VERSION = re.search(r'FIRMWARE_VERSION\[\]\s*=\s*"(\S+)', (ROOT / "lib/config/config.h").read_text(encoding="utf-8")).group(1)
REPOSITORY = f"https://github.com/SandroGrassia/Lilla_Audio_Sampler/blob/v{VERSION}/"


def configure_fonts():
    if os.name == "nt":
        directory = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts"
        names = ("arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf")
    else:
        directory = Path("/usr/share/fonts/truetype/dejavu")
        names = ("DejaVuSans.ttf", "DejaVuSans-Bold.ttf", "DejaVuSans-Oblique.ttf", "DejaVuSans-BoldOblique.ttf")
    for face, filename in zip(("Guide", "GuideBold", "GuideItalic", "GuideBoldItalic"), names):
        pdfmetrics.registerFont(TTFont(face, str(directory / filename)))
    pdfmetrics.registerFontFamily("Guide", normal="Guide", bold="GuideBold", italic="GuideItalic", boldItalic="GuideBoldItalic")


def slug(text):
    return re.sub(r"[^\w\s-]", "", text.lower()).replace(" ", "-")


def inline(text):
    tokens = []

    def hold(markup):
        tokens.append(markup)
        return f"\x01{len(tokens) - 1}\x02"

    text = re.sub(r"`([^`]+)`", lambda m: hold(f'<font name="Courier" size="9">{escape(m[1])}</font>'), text)

    def link(match):
        target = match[2]
        if not target.startswith(("#", "https://", "http://", "mailto:")):
            target = urljoin(REPOSITORY, target)
        return hold(f'<a href="{escape(target, quote=True)}" color="#176566">{inline(match[1])}</a>')

    text = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", link, text)
    text = re.sub(r"<br\s*/?>", lambda m: hold("<br/>"), text)
    text = escape(text)
    text = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", text)
    text = re.sub(r"\*([^*]+)\*", r"<i>\1</i>", text)
    for index, markup in enumerate(tokens):
        text = text.replace(f"\x01{index}\x02", markup)
    return text


class GuideDocument(SimpleDocTemplate):
    def afterFlowable(self, flowable):
        if hasattr(flowable, "bookmark"):
            self.canv.bookmarkPage(flowable.bookmark)
            self.canv.addOutlineEntry(flowable.getPlainText(), flowable.bookmark, flowable.outline_level, False)


def build():
    configure_fonts()
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle("GuideBody", fontName="Guide", fontSize=10, leading=14, spaceAfter=7))
    styles.add(ParagraphStyle("GuideTitle", parent=styles["GuideBody"], fontName="GuideBold", fontSize=25, leading=30, spaceAfter=18))
    styles.add(ParagraphStyle("GuideH2", parent=styles["GuideBody"], fontName="GuideBold", fontSize=19, leading=24, spaceBefore=18, spaceAfter=14, keepWithNext=True))
    styles.add(ParagraphStyle("GuideH3", parent=styles["GuideBody"], fontName="GuideBold", fontSize=12, leading=16, spaceBefore=12, spaceAfter=7, keepWithNext=True))
    styles.add(ParagraphStyle("GuideCell", parent=styles["GuideBody"], fontSize=8.5, leading=11.5, spaceAfter=0))
    styles.add(ParagraphStyle("GuideList", parent=styles["GuideBody"], leftIndent=16, firstLineIndent=0, bulletIndent=0, bulletFontName="Guide", bulletFontSize=10, spaceAfter=5))
    styles.add(ParagraphStyle("GuideQuote", parent=styles["GuideBody"], leftIndent=12, borderColor=colors.HexColor("#176566"), borderWidth=1, borderPadding=8, spaceBefore=7, spaceAfter=12))
    styles.add(ParagraphStyle("GuideCode", fontName="Courier", fontSize=8, leading=11, spaceBefore=6, spaceAfter=9))
    output = ROOT / "User Guide.pdf"
    document = GuideDocument(str(output), pagesize=A4, rightMargin=43, leftMargin=43, topMargin=48, bottomMargin=43, title=f"LILLA User Guide - Firmware {VERSION}", author="Sandro Grassia", allowSplitting=True)
    lines = (ROOT / "User Guide.md").read_text(encoding="utf-8").splitlines()
    story = []
    index = 0
    while index < len(lines):
        line = lines[index].strip()
        index += 1
        if not line or line == "---":
            continue
        heading = re.match(r"^(#{1,4}) (.+)", line)
        if heading:
            level, title = len(heading[1]), heading[2]
            if level == 2:
                story.append(PageBreak() if title in ("Contents", "Getting started") else CondPageBreak(280))
            paragraph = Paragraph(inline(title), styles["GuideTitle" if level == 1 else "GuideH2" if level == 2 else "GuideH3"])
            paragraph.bookmark = slug(title)
            paragraph.outline_level = min(level - 1, 2)
            story.append(paragraph)
            continue
        image = re.search(r'<img src="([^"]+)"[^>]*width="(\d+)%"', line)
        if image:
            source = ROOT / unquote(image[1])
            with PillowImage.open(source) as picture:
                width, height = picture.size
            display_width = document.width * int(image[2]) / 100
            story.append(Image(str(source), width=display_width, height=display_width * height / width, hAlign="LEFT"))
            story.append(Spacer(1, 9))
            continue
        if line.startswith("```"):
            code = []
            while index < len(lines) and not lines[index].startswith("```"):
                code.append(lines[index])
                index += 1
            index += 1
            story.append(Preformatted("\n".join(code), styles["GuideCode"]))
            continue
        if line.startswith("|"):
            rows = [line]
            while index < len(lines) and lines[index].strip().startswith("|"):
                rows.append(lines[index].strip())
                index += 1
            cells = [[cell.strip() for cell in row.strip("|").split("|")] for row in rows if not re.fullmatch(r"[\s|:-]+", row)]
            count = len(cells[0])
            weights = [0.34, 0.66] if count == 2 else [0.27, 0.23, 0.50] if count == 3 else [1 / count] * count
            data = [[Paragraph(("<b>" + inline(cell) + "</b>") if row == 0 else inline(cell), styles["GuideCell"]) for cell in values] for row, values in enumerate(cells)]
            table = Table(data, colWidths=[document.width * weight for weight in weights], repeatRows=1, hAlign="LEFT")
            table.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#e0eded")), ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#f4f6f6")]), ("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, 0), 0.6, colors.HexColor("#799394")), ("LEFTPADDING", (0, 0), (-1, -1), 7), ("RIGHTPADDING", (0, 0), (-1, -1), 7), ("TOPPADDING", (0, 0), (-1, -1), 6), ("BOTTOMPADDING", (0, 0), (-1, -1), 6)]))
            story.extend([table, Spacer(1, 10)])
            continue
        item = re.match(r"^(-|\d+\.) (.+)", line)
        if item:
            story.append(Paragraph(inline(item[2]), styles["GuideList"], bulletText="\u2022" if item[1] == "-" else item[1]))
            continue
        if line.startswith(">"):
            quote = [line.lstrip("> ")]
            while index < len(lines) and lines[index].startswith(">"):
                quote.append(lines[index].lstrip("> "))
                index += 1
            story.append(Paragraph(inline(" ".join(quote)), styles["GuideQuote"]))
            continue
        paragraph = [line]
        while index < len(lines) and lines[index].strip() and not re.match(r"^(#|<img|```|\||>|- |\d+\. )", lines[index]):
            paragraph.append(lines[index].strip())
            index += 1
        story.append(Paragraph(inline(" ".join(paragraph)), styles["GuideBody"]))

    def furniture(canvas, doc):
        canvas.saveState()
        canvas.setFont("Guide", 8)
        canvas.setFillColor(colors.HexColor("#586969"))
        canvas.drawString(doc.leftMargin, A4[1] - 27, f"LILLA User Guide | Firmware {VERSION}")
        canvas.drawRightString(A4[0] - doc.rightMargin, 24, str(doc.page))
        canvas.restoreState()

    document.build(story, onFirstPage=furniture, onLaterPages=furniture)
    print(output)


if __name__ == "__main__":
    build()
