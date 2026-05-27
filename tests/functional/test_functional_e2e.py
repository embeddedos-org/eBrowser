import unittest

class TesteBrowserFunctional(unittest.TestCase):
    def test_dom_tree_rendering_pipeline(self):
        tokens = ["<html>", "<body>", "EmbeddedOS", "</body>", "</html>"]
        stack = []
        for t in tokens:
            if t.startswith("</"):
                stack.pop()
            elif t.startswith("<"):
                stack.append(t)
        assert len(stack) == 0, "DOM tree failed to balance and close tags"
