import { useEffect } from 'react';
import { useBrowserStore } from '@/store/browserStore';

export function useKeyboardShortcuts() {
  const store = useBrowserStore();

  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      const ctrl = e.ctrlKey || e.metaKey;
      const shift = e.shiftKey;
      const alt = e.altKey;

      // Prevent default for browser shortcuts we handle
      const prevent = () => e.preventDefault();

      if (ctrl && e.key === 't') { prevent(); store.openTab(); }
      else if (ctrl && shift && e.key === 'T') { prevent(); store.reopenLastClosed(); }
      else if (ctrl && e.key === 'w') { prevent(); const t = store.getActiveTab(); if (t) store.closeTab(t.id); }
      else if (ctrl && shift && e.key === 'N') { prevent(); store.openIncognito(); }
      else if (ctrl && e.key === 'n') { prevent(); store.openTab(); }
      else if (ctrl && e.key === 'l' || e.key === 'F6') { prevent(); document.getElementById('address-bar-input')?.focus(); }
      else if (ctrl && e.key === 'r' || e.key === 'F5') { prevent(); store.reload(); }
      else if (ctrl && shift && e.key === 'R') { prevent(); store.reload(); }
      else if (alt && e.key === 'ArrowLeft') { prevent(); store.goBack(); }
      else if (alt && e.key === 'ArrowRight') { prevent(); store.goForward(); }
      else if (ctrl && e.key === 'd') { prevent(); /* bookmark */ }
      else if (ctrl && e.key === 'h') { prevent(); store.setSidebarPanel('history'); }
      else if (ctrl && e.key === 'j') { prevent(); store.toggleDownloadsPanel(); }
      else if (ctrl && shift && e.key === 'B') { prevent(); store.updateSettings({ showBookmarksBar: !store.settings.showBookmarksBar }); }
      else if (ctrl && shift && e.key === 'J' || e.key === 'F12') { prevent(); store.toggleDevTools(); }
      else if (ctrl && e.key === 'f') { prevent(); store.toggleFindBar(); }
      else if (ctrl && e.key === 'k') { prevent(); store.toggleCommandPalette(); }
      else if (ctrl && e.key === '+' || ctrl && e.key === '=') { prevent(); const t = store.getActiveTab(); if (t) store.updateTab(t.id, { zoom: Math.min(t.zoom + 10, 300) }); }
      else if (ctrl && e.key === '-') { prevent(); const t = store.getActiveTab(); if (t) store.updateTab(t.id, { zoom: Math.max(t.zoom - 10, 25) }); }
      else if (ctrl && e.key === '0') { prevent(); const t = store.getActiveTab(); if (t) store.updateTab(t.id, { zoom: 100 }); }
      else if (e.key === 'F11') { prevent(); store.toggleFullscreen(); }
      else if (e.key === 'Escape') {
        if (store.showCommandPalette) { prevent(); store.toggleCommandPalette(); }
        else if (store.showFindBar) { prevent(); store.toggleFindBar(); }
      }
      // Tab switching with Ctrl+1-9
      else if (ctrl && e.key >= '1' && e.key <= '9') {
        prevent();
        const idx = parseInt(e.key) - 1;
        if (idx < store.tabs.length) store.setActiveTab(store.tabs[idx].id);
      }
    };

    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [store]);
}
