import React, { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { X, Star, Clock, Zap, BookOpen, FileText, Key, ChevronRight } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { useBrowserStore } from '@/store/browserStore';
import { historyDB, bookmarkDB, type HistoryEntry, type Bookmark } from '@/utils/database';

export default function Sidebar() {
  const { t } = useTranslation();
  const { sidebarPanel, setSidebarPanel, getActiveTab, navigateTo } = useBrowserStore();
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [bookmarks, setBookmarks] = useState<Bookmark[]>([]);

  useEffect(() => {
    if (sidebarPanel === 'history') historyDB.getRecent(50).then(setHistory);
    if (sidebarPanel === 'bookmarks') bookmarkDB.getAll().then(setBookmarks);
  }, [sidebarPanel]);

  if (!sidebarPanel) return null;

  const handleNavigate = (url: string) => {
    const tab = getActiveTab();
    if (tab) navigateTo(url, tab.id);
  };

  const panels: Record<string, { icon: React.ReactNode; title: string; content: React.ReactNode }> = {
    bookmarks: {
      icon: <Star size={16} className="text-yellow-500" />,
      title: t('bookmarks.title'),
      content: (
        <div className="space-y-0.5">
          {bookmarks.map(bm => (
            <button key={bm.id} onClick={() => handleNavigate(bm.url)}
              className="w-full flex items-center gap-2 px-3 py-2 rounded-lg hover:bg-gray-100 dark:hover:bg-gray-700 text-left transition-colors"
            >
              {bm.favicon && <img src={bm.favicon} alt="" className="w-4 h-4 rounded-sm flex-shrink-0" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />}
              <span className="text-sm text-gray-700 dark:text-gray-300 truncate">{bm.title}</span>
            </button>
          ))}
          {bookmarks.length === 0 && <div className="text-center py-8 text-gray-400 text-sm">No bookmarks yet</div>}
        </div>
      ),
    },
    history: {
      icon: <Clock size={16} className="text-gray-500" />,
      title: t('history.title'),
      content: (
        <div className="space-y-0.5">
          {history.map(entry => (
            <button key={entry.id} onClick={() => handleNavigate(entry.url)}
              className="w-full flex items-center gap-2 px-3 py-2 rounded-lg hover:bg-gray-100 dark:hover:bg-gray-700 text-left transition-colors"
            >
              {entry.favicon && <img src={entry.favicon} alt="" className="w-4 h-4 rounded-sm flex-shrink-0" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />}
              <div className="flex-1 min-w-0">
                <div className="text-sm text-gray-700 dark:text-gray-300 truncate">{entry.title || entry.url}</div>
                <div className="text-xs text-gray-400 truncate">{entry.url}</div>
              </div>
            </button>
          ))}
          {history.length === 0 && <div className="text-center py-8 text-gray-400 text-sm">No history yet</div>}
        </div>
      ),
    },
    extensions: {
      icon: <Zap size={16} className="text-purple-500" />,
      title: t('extensions.title'),
      content: (
        <div className="p-3">
          <button onClick={() => handleNavigate('ebrowser://extensions')} className="btn-primary w-full gap-2 text-sm">
            <Zap size={12} />
            Manage Extensions
          </button>
          <div className="mt-4 text-sm text-gray-500 dark:text-gray-400">
            <p>Extensions add features to eBrowser. Compatible with Chrome WebExtensions V3.</p>
          </div>
        </div>
      ),
    },
  };

  const panel = panels[sidebarPanel];
  if (!panel) return null;

  return (
    <motion.div
      initial={{ width: 0, opacity: 0 }}
      animate={{ width: 280, opacity: 1 }}
      exit={{ width: 0, opacity: 0 }}
      transition={{ duration: 0.2 }}
      className="flex-shrink-0 border-r border-gray-200 dark:border-gray-700 bg-white dark:bg-gray-800 overflow-hidden flex flex-col"
    >
      {/* Header */}
      <div className="flex items-center justify-between px-4 py-3 border-b border-gray-200 dark:border-gray-700 flex-shrink-0">
        <div className="flex items-center gap-2">
          {panel.icon}
          <span className="text-sm font-semibold text-gray-900 dark:text-gray-100">{panel.title}</span>
        </div>
        <button onClick={() => setSidebarPanel(null)} className="icon-btn w-6 h-6">
          <X size={14} />
        </button>
      </div>

      {/* Content */}
      <div className="flex-1 overflow-y-auto p-2">
        {panel.content}
      </div>
    </motion.div>
  );
}
