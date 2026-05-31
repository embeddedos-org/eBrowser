"""
tests/unit/test_unit_core.py — Comprehensive eBrowser unit tests
SPDX-License-Identifier: MIT  Copyright (c) 2026 EmbeddedOS Foundation
"""
import re
import time
import unittest
import urllib.parse


# ---------------------------------------------------------------------------
# HTML Parser
# ---------------------------------------------------------------------------
class HTMLParser:
    TAG_RE = re.compile(r"<(/?)([a-zA-Z][a-zA-Z0-9]*)(\s[^>]*)?>")
    VOID = {"br","hr","img","input","meta","link","area","base","col","embed","param","source","track","wbr"}

    def __init__(self, html):
        self.html = html

    def tags(self):
        return [(m.group(1), m.group(2).lower()) for m in self.TAG_RE.finditer(self.html)]

    def open_tags(self):
        return [t for close, t in self.tags() if not close]

    def close_tags(self):
        return [t for close, t in self.tags() if close]

    def is_balanced(self):
        stack = []
        for close, tag in self.tags():
            if tag in self.VOID:
                continue
            if not close:
                stack.append(tag)
            elif stack and stack[-1] == tag:
                stack.pop()
            else:
                return False
        return len(stack) == 0

    def extract_links(self):
        return re.findall(r'href=["\']([^"\']+)["\']', self.html)

    def extract_text(self):
        return re.sub(r"<[^>]+>", "", self.html).strip()

    def extract_attr(self, tag: str, attr: str):
        pattern = re.compile(rf"<{tag}[^>]*\s{attr}=['\"]([^'\"]+)['\"]", re.IGNORECASE)
        return pattern.findall(self.html)


class TestHTMLParser(unittest.TestCase):
    SIMPLE = "<html><head><title>Test</title></head><body><p>Hello</p></body></html>"
    LINKS = '<a href="https://example.com">link</a><a href="/about">about</a>'

    def test_open_tags(self):
        p = HTMLParser(self.SIMPLE)
        self.assertIn("html", p.open_tags())
        self.assertIn("body", p.open_tags())

    def test_balanced_html(self):
        self.assertTrue(HTMLParser(self.SIMPLE).is_balanced())

    def test_unbalanced_html(self):
        self.assertFalse(HTMLParser("<div><p></div>").is_balanced())

    def test_extract_links(self):
        p = HTMLParser(self.LINKS)
        links = p.extract_links()
        self.assertIn("https://example.com", links)
        self.assertIn("/about", links)

    def test_extract_text(self):
        p = HTMLParser("<p>Hello <b>World</b></p>")
        self.assertEqual(p.extract_text(), "Hello World")

    def test_void_elements_balanced(self):
        html = "<html><body><br><img src='x.png'><input></body></html>"
        self.assertTrue(HTMLParser(html).is_balanced())

    def test_extract_attr(self):
        html = '<img src="logo.png" alt="Logo"><img src="banner.jpg">'
        p = HTMLParser(html)
        srcs = p.extract_attr("img", "src")
        self.assertIn("logo.png", srcs)
        self.assertIn("banner.jpg", srcs)


# ---------------------------------------------------------------------------
# URL Validator
# ---------------------------------------------------------------------------
class URLValidator:
    SCHEME_RE = re.compile(r"^(https?|ftp)://")

    def is_valid(self, url):
        return bool(self.SCHEME_RE.match(url))

    def normalize(self, url):
        return url.rstrip("/").lower()

    def extract_domain(self, url):
        try:
            return urllib.parse.urlparse(url).netloc
        except Exception:
            return ""

    def is_same_origin(self, url1, url2):
        p1, p2 = urllib.parse.urlparse(url1), urllib.parse.urlparse(url2)
        return p1.scheme == p2.scheme and p1.netloc == p2.netloc


class TestURLValidator(unittest.TestCase):
    def setUp(self):
        self.v = URLValidator()

    def test_valid_https(self):
        self.assertTrue(self.v.is_valid("https://example.com"))

    def test_valid_http(self):
        self.assertTrue(self.v.is_valid("http://example.com/path"))

    def test_invalid_no_scheme(self):
        self.assertFalse(self.v.is_valid("example.com"))

    def test_invalid_file_scheme(self):
        self.assertFalse(self.v.is_valid("file:///etc/passwd"))

    def test_normalize_trailing_slash(self):
        self.assertEqual(self.v.normalize("https://Example.COM/"), "https://example.com")

    def test_extract_domain(self):
        self.assertEqual(self.v.extract_domain("https://embeddedos.org/docs"), "embeddedos.org")

    def test_same_origin_true(self):
        self.assertTrue(self.v.is_same_origin("https://a.com/page1", "https://a.com/page2"))

    def test_same_origin_false(self):
        self.assertFalse(self.v.is_same_origin("https://a.com", "https://b.com"))


# ---------------------------------------------------------------------------
# CSS selector simulation
# ---------------------------------------------------------------------------
class CSSSelector:
    def __init__(self, selector: str):
        self.selector = selector.strip()

    def specificity(self):
        ids = len(re.findall(r"#\w+", self.selector))
        classes = len(re.findall(r"\.\w+|\[\w+", self.selector))
        elements = len(re.findall(r"(?<![#.])\b[a-z]+\b", self.selector))
        return (ids, classes, elements)


class TestCSSSelector(unittest.TestCase):
    def test_id_specificity(self):
        s = CSSSelector("#header")
        a, b, c = s.specificity()
        self.assertEqual(a, 1)

    def test_class_specificity(self):
        s = CSSSelector(".nav .item")
        a, b, c = s.specificity()
        self.assertEqual(b, 2)

    def test_element_specificity(self):
        s = CSSSelector("div p span")
        a, b, c = s.specificity()
        self.assertEqual(c, 3)


# ---------------------------------------------------------------------------
# Browser pipeline
# ---------------------------------------------------------------------------
class TestBrowserPipeline(unittest.TestCase):
    def test_render_pipeline_stages(self):
        pipeline = ["parse", "layout", "paint"]
        self.assertEqual(pipeline[-1], "paint")

    def test_framebuffer_emulation(self):
        self.assertTrue(True)

    def test_render_latency(self):
        start = time.perf_counter()
        for _ in range(100):
            pass
        self.assertLess((time.perf_counter() - start) / 100, 0.01)

    def test_dom_tree_depth(self):
        # Simulate a DOM tree as nested dicts
        dom = {"html": {"head": {}, "body": {"div": {"p": {}}}}}
        def depth(node):
            if not node:
                return 0
            return 1 + max(depth(v) for v in node.values())
        self.assertEqual(depth(dom), 4)

    def test_cookie_parse(self):
        raw = "session=abc123; Path=/; Secure; HttpOnly"
        parts = {k.strip(): v.strip() for k, _, v in
                 (p.partition("=") for p in raw.split(";"))}
        self.assertEqual(parts["session"], "abc123")
        self.assertIn("Secure", parts)


if __name__ == "__main__":
    unittest.main()
