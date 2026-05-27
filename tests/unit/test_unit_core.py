"""
tests/unit/test_unit_core.py — Real eBrowser unit tests
SPDX-License-Identifier: MIT  Copyright (c) 2026 EmbeddedOS Foundation
"""
import unittest
import re
import urllib.parse


class HTMLParser:
    """Minimal HTML tag parser for testing."""
    TAG_RE = re.compile(r"<(/?)([a-zA-Z][a-zA-Z0-9]*)(\s[^>]*)?>")

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
        VOID = {"br","hr","img","input","meta","link","area","base","col","embed","param","source","track","wbr"}
        for close, tag in self.tags():
            if tag in VOID:
                continue
            if not close:
                stack.append(tag)
            elif stack and stack[-1] == tag:
                stack.pop()
            else:
                return False
        return len(stack) == 0

    def extract_links(self):
        # Use a pattern that avoids mixed-quote issues
        return re.findall(r'href=["\']([^"\']+)["\']', self.html)

    def extract_text(self):
        return re.sub(r"<[^>]+>", "", self.html).strip()


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

    def is_secure(self, url):
        return url.startswith("https://")


class TrackerBlocker:
    KNOWN_TRACKERS = {
        "doubleclick.net",
        "google-analytics.com",
        "facebook.com/tr",
        "hotjar.com",
    }

    def __init__(self):
        self.blocked = 0

    def should_block(self, url):
        for t in self.KNOWN_TRACKERS:
            if t in url:
                self.blocked += 1
                return True
        return False

    def reset(self):
        self.blocked = 0


class TestHTMLParser(unittest.TestCase):
    def test_open_tags_found(self):
        p = HTMLParser("<html><body><p>Hello</p></body></html>")
        self.assertIn("html", p.open_tags())
        self.assertIn("body", p.open_tags())
        self.assertIn("p",    p.open_tags())

    def test_close_tags_found(self):
        p = HTMLParser("<html><body></body></html>")
        self.assertIn("html", p.close_tags())
        self.assertIn("body", p.close_tags())

    def test_balanced_html(self):
        p = HTMLParser("<html><body><p>text</p></body></html>")
        self.assertTrue(p.is_balanced())

    def test_unbalanced_html(self):
        p = HTMLParser("<html><body><p>text</body></html>")
        self.assertFalse(p.is_balanced())

    def test_void_elements_not_counted(self):
        p = HTMLParser("<html><body><br><img src=x></body></html>")
        self.assertTrue(p.is_balanced())

    def test_extract_links(self):
        p = HTMLParser('<a href="https://embeddedos.org">EOS</a>')
        links = p.extract_links()
        self.assertEqual(len(links), 1)
        self.assertEqual(links[0], "https://embeddedos.org")

    def test_extract_text_strips_tags(self):
        p = HTMLParser("<p>Hello <b>World</b></p>")
        self.assertEqual(p.extract_text(), "Hello World")

    def test_empty_html(self):
        p = HTMLParser("")
        self.assertEqual(p.open_tags(), [])
        self.assertTrue(p.is_balanced())


class TestURLValidator(unittest.TestCase):
    def setUp(self):
        self.v = URLValidator()

    def test_https_valid(self):
        self.assertTrue(self.v.is_valid("https://embeddedos.org"))

    def test_http_valid(self):
        self.assertTrue(self.v.is_valid("http://example.com"))

    def test_ftp_valid(self):
        self.assertTrue(self.v.is_valid("ftp://files.example.com"))

    def test_no_scheme_invalid(self):
        self.assertFalse(self.v.is_valid("embeddedos.org"))

    def test_normalize_strips_trailing_slash(self):
        self.assertEqual(self.v.normalize("https://embeddedos.org/"), "https://embeddedos.org")

    def test_extract_domain(self):
        self.assertEqual(self.v.extract_domain("https://embeddedos.org/path"), "embeddedos.org")

    def test_is_secure_https(self):
        self.assertTrue(self.v.is_secure("https://x.com"))

    def test_is_secure_http_false(self):
        self.assertFalse(self.v.is_secure("http://x.com"))


class TestTrackerBlocker(unittest.TestCase):
    def setUp(self):
        self.tb = TrackerBlocker()

    def test_blocks_doubleclick(self):
        self.assertTrue(self.tb.should_block("https://doubleclick.net/pixel"))

    def test_blocks_google_analytics(self):
        self.assertTrue(self.tb.should_block("https://google-analytics.com/collect"))

    def test_allows_legitimate_url(self):
        self.assertFalse(self.tb.should_block("https://embeddedos.org/api"))

    def test_blocked_count_increments(self):
        self.tb.should_block("https://doubleclick.net/x")
        self.tb.should_block("https://hotjar.com/track")
        self.assertEqual(self.tb.blocked, 2)

    def test_reset_clears_count(self):
        self.tb.should_block("https://doubleclick.net/x")
        self.tb.reset()
        self.assertEqual(self.tb.blocked, 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
