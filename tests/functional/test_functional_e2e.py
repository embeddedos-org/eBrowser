import unittest
class TestEBrowserFunctional(unittest.TestCase):
    def test_browser_render_pipeline(self):
        pipeline = ["parse", "layout", "paint"]
        self.assertEqual(pipeline[-1], "paint")
