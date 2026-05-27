import unittest

class TesteBrowserUnit(unittest.TestCase):
    def test_html_tag_tokenizer(self):
        html = "<html><body><h1>EmbeddedOS</h1></body></html>"
        tokens = []
        # Simple tag tokenizer
        import re
        tokens = re.findall(r'<[^>]+>|[^<]+', html)
        assert "<html>" in tokens
        assert "EmbeddedOS" in tokens
        assert "</html>" in tokens
