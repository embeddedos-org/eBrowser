import React, { useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { useBrowserStore } from '@/store/browserStore';
import TabBar from './TabBar';
import Toolbar from './Toolbar';
import BookmarksBar from './BookmarksBar';
import WebView from './WebView';
import Sidebar from './Sidebar';
import DevTools from './DevTools';
import FindBar from './FindBar';
import StatusBar from './StatusBar';
import DownloadsPanel from './DownloadsPanel';

export default function BrowserShell() {
  const {
    tabs, activeTabId, settings, isFullscreen, isZenMode,
    showDevTools, showFindBar, showDownloadsPanel, sidebarOpen,
    sidebarPanel, setActiveTab
  } = useBrowserStore();

  const activeTab = tabs.find(t => t.id === activeTabId) ?? tabs[0];

  // Initialize active tab if not set
  useEffect(() => {
    if (!activeTabId && tabs.length > 0) {
      setActiveTab(tabs[0].id);
    }
  }, [activeTabId, tabs, setActiveTab]);

  const isIncognito = activeTab?.isIncognito ?? false;

  return (
    <div
      className={`flex flex-col h-full overflow-hidden ${isIncognito ? 'incognito-mode' : ''}`}
      data-theme={settings.theme}
    >
      {/* Tab Bar — hidden in zen/fullscreen mode */}
      <AnimatePresence>
        {!isFullscreen && !isZenMode && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.15 }}
          >
            <TabBar />
          </motion.div>
        )}
      </AnimatePresence>

      {/* Toolbar — hidden in fullscreen mode */}
      <AnimatePresence>
        {!isFullscreen && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.15 }}
          >
            <Toolbar />
          </motion.div>
        )}
      </AnimatePresence>

      {/* Bookmarks Bar */}
      <AnimatePresence>
        {settings.showBookmarksBar && !isFullscreen && !isZenMode && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.15 }}
          >
            <BookmarksBar />
          </motion.div>
        )}
      </AnimatePresence>

      {/* Find Bar */}
      <AnimatePresence>
        {showFindBar && (
          <motion.div
            initial={{ y: -20, opacity: 0 }}
            animate={{ y: 0, opacity: 1 }}
            exit={{ y: -20, opacity: 0 }}
            transition={{ duration: 0.15 }}
          >
            <FindBar />
          </motion.div>
        )}
      </AnimatePresence>

      {/* Main content area */}
      <div className="flex flex-1 overflow-hidden">
        {/* Sidebar */}
        <AnimatePresence>
          {sidebarOpen && sidebarPanel && (
            <Sidebar />
          )}
        </AnimatePresence>

        {/* WebView area */}
        <div className="flex-1 flex flex-col overflow-hidden">
          {/* Render all tabs but only show active one */}
          <div className="flex-1 relative overflow-hidden">
            {tabs.map(tab => (
              <div
                key={tab.id}
                className={`absolute inset-0 ${tab.id === activeTabId ? 'block' : 'hidden'}`}
              >
                <WebView tab={tab} />
              </div>
            ))}
          </div>

          {/* DevTools Panel */}
          <AnimatePresence>
            {showDevTools && (
              <DevTools />
            )}
          </AnimatePresence>
        </div>
      </div>

      {/* Downloads Panel */}
      <AnimatePresence>
        {showDownloadsPanel && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.2 }}
          >
            <DownloadsPanel />
          </motion.div>
        )}
      </AnimatePresence>

      {/* Status Bar */}
      {settings.showStatusBar && !isFullscreen && (
        <StatusBar />
      )}
    </div>
  );
}
