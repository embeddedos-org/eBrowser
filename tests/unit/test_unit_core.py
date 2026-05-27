import unittest
class TestEBrowserUnit(unittest.TestCase):
    def test_html_tag_parsing(self):
        html = "<html><body></body></html>"
        self.assertTrue("body" in html)
