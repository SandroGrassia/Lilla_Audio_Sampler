"""Extract firmware definitions for host tests without target placement attributes."""
import re


def method(source, signature):
    """Return a top-level definition, allowing FLASHMEM/FASTRUN before its signature."""
    pattern = r'^(?:(?:FLASHMEM|FASTRUN)[ \t]+)?(' + re.escape(signature) + r'[ \t]*(?://[^\n]*)?\n\{.*?^\})'
    match = re.search(pattern, source, re.M | re.S)
    if match is None:
        raise ValueError(f'Production definition not found: {signature}')
    return match.group(1) + '\n'
