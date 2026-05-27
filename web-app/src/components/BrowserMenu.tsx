import React from 'react';
import { motion } from 'framer-motion';
import {
  Plus, BookOpen, Clock, Download, Settings, Printer, Share2,
  ZoomIn, ZoomOut, Maximize2, EyeOff, Zap, Shield, Globe,
  FileText, Key, MapPin, Star, Code2, RefreshCw, X
} from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { useBrowserStore } from '@/store/browserStore';

interface Props {
  onClose: () => void;
}

export default function BrowserMenu({ onClose }: Props) {
  const { t } = useTranslation();
  const store = useBrowserStore();
  const activeTab = store.getActiveTab();

  const navigate = (url: string) => {
    if (activeTab) store.navigateTo(url, activeTab.id);
    onClose();
  };

  const menuItems = [
    {
      group: 'Navigation',
      items: [
        { icon: <Plus size={14} />, label: t('browser.newTab'), shortcut: 'Ctrl+T', action: () => { store.openTab(); onClose(); } },
        { icon: <EyeOff size={14} />, label: t('browser.incognito'), shortcut: 'Ctrl+Shift+N', action: () => { store.openIncognito(); onClose(); } },
        { icon: <RefreshCw size={14} />, label: t('browser.reload'), shortcut: 'Ctrl+R', action: () => { store.reload(); onClose(); } },
      ],
    },
    {
      group: 'View',
      items: [
        { icon: <ZoomIn size={14} />, label: t('browser.zoomIn'), shortcut: 'Ctrl++', action: () => { if (activeTab) store.updateTab(activeTab.id, { zoom: Math.min(activeTab.zoom + 10, 300) }); onClose(); } },
        { icon: <ZoomOut size={14} />, label: t('browser.zoomOut'), shortcut: 'Ctrl+-', action: () => { if (activeTab) store.updateTab(activeTab.id, { zoom: Math.max(activeTab.zoom - 10, 25) }); onClose(); } },
        { icon: <Maximize2 size={14} />, label: t('browser.fullscreen'), shortcut: 'F11', action: () => { document.documentElement.requestFullscreen?.(); onClose(); } },
        { icon: <BookOpen size={14} />, label: t('browser.readingMode'), shortcut: 'Alt+R', action: () => { if (activeTab) store.updateTab(activeTab.id, { readingMode: !activeTab.readingMode }); onClose(); } },
      ],
    },
    {
      group: 'Tools',
      items: [
        { icon: <Star size={14} />, label: t('browser.bookmarks'), shortcut: 'Ctrl+Shift+B', action: () => navigate('ebrowser://bookmarks') },
        { icon: <Clock size={14} />, label: t('browser.history'), shortcut: 'Ctrl+H', action: () => navigate('ebrowser://history') },
        { icon: <Download size={14} />, label: t('browser.downloads'), shortcut: 'Ctrl+J', action: () => navigate('ebrowser://downloads') },
        { icon: <Key size={14} />, label: 'Passwords', action: () => navigate('ebrowser://passwords') },
        { icon: <FileText size={14} />, label: 'Notes', action: () => navigate('ebrowser://notes') },
        { icon: <BookOpen size={14} />, label: 'Reading List', action: () => navigate('ebrowser://reading-list') },
        { icon: <MapPin size={14} />, label: 'GPS & Location', action: () => navigate('ebrowser://gps') },
      ],
    },
    {
      group: 'Advanced',
      items: [
        { icon: <Shield size={14} />, label: t('browser.privacy'), action: () => navigate('ebrowser://privacy') },
        { icon: <Zap size={14} />, label: t('browser.extensions'), action: () => navigate('ebrowser://extensions') },
        { icon: <Code2 size={14} />, label: t('browser.devtools'), shortcut: 'F12', action: () => { store.toggleDevTools(); onClose(); } },
        { icon: <Printer size={14} />, label: t('browser.print'), shortcut: 'Ctrl+P', action: () => { window.print(); onClose(); } },
        { icon: <Share2 size={14} />, label: t('browser.share'), action: () => { navigator.share?.({ url: activeTab?.url, title: activeTab?.title }); onClose(); } },
        { icon: <Settings size={14} />, label: t('browser.settings'), shortcut: 'Ctrl+,', action: () => navigate('ebrowser://settings') },
      ],
    },
  ];

  return (
    <>
      <div className="fixed inset-0 z-40" onClick={onClose} />
      <motion.div
        initial={{ opacity: 0, scale: 0.95, y: -5 }}
        animate={{ opacity: 1, scale: 1, y: 0 }}
        exit={{ opacity: 0, scale: 0.95, y: -5 }}
        transition={{ duration: 0.1 }}
        className="absolute right-0 top-full mt-1 w-64 bg-white dark:bg-gray-800 rounded-xl shadow-dropdown border border-gray-200 dark:border-gray-700 overflow-hidden z-50"
      >
        {menuItems.map((group, gi) => (
          <div key={group.group}>
            {gi > 0 && <div className="h-px bg-gray-100 dark:bg-gray-700" />}
            <div className="py-1">
              <div className="px-3 py-1 text-[10px] font-semibold text-gray-400 uppercase tracking-wider">
                {group.group}
              </div>
              {group.items.map((item) => (
                <button
                  key={item.label}
                  onClick={item.action}
                  className="w-full flex items-center gap-2.5 px-3 py-2 text-sm text-gray-700 dark:text-gray-300 hover:bg-gray-100 dark:hover:bg-gray-700 transition-colors"
                >
                  <span className="text-gray-400 flex-shrink-0">{item.icon}</span>
                  <span className="flex-1 text-left">{item.label}</span>
                  {item.shortcut && (
                    <span className="text-[10px] text-gray-400 font-mono">{item.shortcut}</span>
                  )}
                </button>
              ))}
            </div>
          </div>
        ))}
      </motion.div>
    </>
  );
}
