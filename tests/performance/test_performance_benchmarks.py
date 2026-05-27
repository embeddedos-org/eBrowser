import unittest

class TesteBrowserPerformance(unittest.TestCase):
    import time
    def test_page_render_time(self):
        import time
        start = time.perf_counter()
        # Simulate rendering simple page layout
        for _ in range(500):
            _ = "div" + "span" + "p"
        end = time.perf_counter()
        render_ms = (end - start) * 1000
        assert render_ms < 10, f"Render time {render_ms:.1f}ms exceeds 10ms SLA"
