import React, { useState, useEffect, useRef } from 'react';
import { motion } from 'framer-motion';
import { Search, Command, ArrowRight, Clock, Star, Settings, Shield, Download, Zap, MapPin, Key, FileText, BookOpen } from 'lucide-react';
import { useBrowserStore } from '@/store/browserStore';
import { normalizeInput } from '@/utils/url';

interface CommandItem {
  id: string;
  icon: React.ReactNode;
  label: string;
  description?: string;
  shortcut?: string;
  action: () => void;
  category: string;
}

export default function CommandPalette() {
  const store = useBrowserStore();
  const [query, setQuery] = useState('');
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    inputRef.current?.focus();
  }, []);

  const navigate = (url: string) => {
    const tab = store.getActiveTab();
    if (tab) store.navigateTo(url, tab.id);
    store.toggleCommandPalette();
  };

  const ALL_COMMANDS: CommandItem[] = [
    { id: 'new-tab', icon: <Command size={14} />, label: 'New Tab', shortcut: 'Ctrl+T', category: 'Browser', action: () => { store.openTab(); store.toggleCommandPalette(); } },
    { id: 'incognito', icon: <Shield size={14} />, label: 'New Incognito Tab', shortcut: 'Ctrl+Shift+N', category: 'Browser', action: () => { store.openIncognito(); store.toggleCommandPalette(); } },
    { id: 'history', icon: <Clock size={14} />, label: 'Open History', shortcut: 'Ctrl+H', category: 'Pages', action: () => navigate('ebrowser://history') },
    { id: 'bookmarks', icon: <Star size={14} />, label: 'Open Bookmarks', shortcut: 'Ctrl+Shift+B', category: 'Pages', action: () => navigate('ebrowser://bookmarks') },
    { id: 'downloads', icon: <Download size={14} />, label: 'Open Downloads', shortcut: 'Ctrl+J', category: 'Pages', action: () => navigate('ebrowser://downloads') },
    { id: 'settings', icon: <Settings size={14} />, label: 'Open Settings', shortcut: 'Ctrl+,', category: 'Pages', action: () => navigate('ebrowser://settings') },
    { id: 'extensions', icon: <Zap size={14} />, label: 'Manage Extensions', category: 'Pages', action: () => navigate('ebrowser://extensions') },
    { id: 'privacy', icon: <Shield size={14} />, label: 'Privacy & Security', category: 'Pages', action: () => navigate('ebrowser://privacy') },
    { id: 'gps', icon: <MapPin size={14} />, label: 'GPS & Location', category: 'Pages', action: () => navigate('ebrowser://gps') },
    { id: 'passwords', icon: <Key size={14} />, label: 'Saved Passwords', category: 'Pages', action: () => navigate('ebrowser://passwords') },
    { id: 'notes', icon: <FileText size={14} />, label: 'Notes', category: 'Pages', action: () => navigate('ebrowser://notes') },
    { id: 'reading-list', icon: <BookOpen size={14} />, label: 'Reading List', category: 'Pages', action: () => navigate('ebrowser://reading-list') },
    { id: 'devtools', icon: <Command size={14} />, label: 'Toggle DevTools', shortcut: 'F12', category: 'Developer', action: () => { store.toggleDevTools(); store.toggleCommandPalette(); } },
    { id: 'dark-mode', icon: <Settings size={14} />, label: 'Toggle Dark Mode', category: 'Appearance', action: () => { store.updateSettings({ theme: store.settings.theme === 'dark' ? 'light' : 'dark' }); store.toggleCommandPalette(); } },
    { id: 'zen-mode', icon: <Command size={14} />, label: 'Toggle Zen Mode', category: 'View', action: () => { store.toggleZenMode(); store.toggleCommandPalette(); } },
  ];

  const filtered = query
    ? ALL_COMMANDS.filter(c =>
        c.label.toLowerCase().includes(query.toLowerCase()) ||
        c.category.toLowerCase().includes(query.toLowerCase())
      )
    : ALL_COMMANDS;

  const grouped = filtered.reduce<Record<string, CommandItem[]>>((acc, item) => {
    if (!acc[item.category]) acc[item.category] = [];
    acc[item.category].push(item);
    return acc;
  }, {});

  return (
    <>
      <div className="fixed inset-0 bg-black/50 z-50 backdrop-blur-sm" onClick={() => store.toggleCommandPalette()} />
      <motion.div
        initial={{ opacity: 0, scale: 0.95, y: -20 }}
        animate={{ opacity: 1, scale: 1, y: 0 }}
        exit={{ opacity: 0, scale: 0.95, y: -20 }}
        transition={{ duration: 0.15 }}
        className="fixed top-1/4 left-1/2 -translate-x-1/2 w-full max-w-lg bg-white dark:bg-gray-800 rounded-2xl shadow-2xl border border-gray-200 dark:border-gray-700 overflow-hidden z-50"
      >
        {/* Search input */}
        <div className="flex items-center gap-3 px-4 py-3 border-b border-gray-200 dark:border-gray-700">
          <Search size={16} className="text-gray-400 flex-shrink-0" />
          <input
            ref={inputRef}
            type="text"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            onKeyDown={(e) => e.key === 'Escape' && store.toggleCommandPalette()}
            placeholder="Type a command or search..."
            className="flex-1 bg-transparent outline-none text-sm text-gray-900 dark:text-gray-100 placeholder-gray-400"
          />
          <kbd className="text-xs text-gray-400 bg-gray-100 dark:bg-gray-700 px-1.5 py-0.5 rounded">Esc</kbd>
        </div>

        {/* Commands */}
        <div className="max-h-80 overflow-y-auto p-2">
          {Object.entries(grouped).map(([category, items]) => (
            <div key={category} className="mb-2">
              <div className="px-2 py-1 text-[10px] font-semibold text-gray-400 uppercase tracking-wider">{category}</div>
              {items.map(item => (
                <button
                  key={item.id}
                  onClick={item.action}
                  className="w-full flex items-center gap-3 px-3 py-2 rounded-lg hover:bg-gray-100 dark:hover:bg-gray-700 transition-colors text-left"
                >
                  <span className="text-gray-400 flex-shrink-0">{item.icon}</span>
                  <span className="flex-1 text-sm text-gray-900 dark:text-gray-100">{item.label}</span>
                  {item.shortcut && (
                    <kbd className="text-[10px] text-gray-400 bg-gray-100 dark:bg-gray-700 px-1.5 py-0.5 rounded font-mono">{item.shortcut}</kbd>
                  )}
                  <ArrowRight size={12} className="text-gray-300 dark:text-gray-600 flex-shrink-0" />
                </button>
              ))}
            </div>
          ))}
          {filtered.length === 0 && (
            <div className="text-center py-8 text-gray-400 text-sm">No commands found</div>
          )}
        </div>
      </motion.div>
    </>
  );
}
