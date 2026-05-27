import React from 'react';
import { motion } from 'framer-motion';
import { RefreshCw, X, Copy, Pin, PinOff, Volume2, VolumeX, ExternalLink, BookOpen } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { useBrowserStore, type Tab } from '@/store/browserStore';

interface Props {
  tab: Tab;
  x: number;
  y: number;
  onClose: () => void;
}

export default function TabContextMenu({ tab, x, y, onClose }: Props) {
  const { t } = useTranslation();
  const store = useBrowserStore();

  const items = [
    { icon: <RefreshCw size={13} />, label: t('browser.reload'), action: () => store.reload(tab.id) },
    { icon: <Copy size={13} />, label: t('browser.duplicateTab'), action: () => store.duplicateTab(tab.id) },
    { icon: <ExternalLink size={13} />, label: t('browser.openInNewWindow'), action: () => window.open(tab.url, '_blank') },
    { separator: true },
    { icon: tab.isPinned ? <PinOff size={13} /> : <Pin size={13} />, label: tab.isPinned ? t('browser.unpin') : t('browser.pin'), action: () => store.updateTab(tab.id, { isPinned: !tab.isPinned }) },
    { icon: tab.isMuted ? <Volume2 size={13} /> : <VolumeX size={13} />, label: tab.isMuted ? t('browser.unmute') : t('browser.mute'), action: () => store.updateTab(tab.id, { isMuted: !tab.isMuted }) },
    { icon: <BookOpen size={13} />, label: 'Add to Reading List', action: () => { /* reading list */ } },
    { separator: true },
    { icon: <X size={13} />, label: t('browser.closeTab'), action: () => store.closeTab(tab.id), danger: true },
    { icon: <X size={13} />, label: t('browser.closeOtherTabs'), action: () => store.closeOtherTabs(tab.id), danger: true },
  ];

  // Position within viewport
  const adjustedX = Math.min(x, window.innerWidth - 200);
  const adjustedY = Math.min(y, window.innerHeight - 300);

  return (
    <>
      <div className="fixed inset-0 z-40" onClick={onClose} />
      <motion.div
        initial={{ opacity: 0, scale: 0.95 }}
        animate={{ opacity: 1, scale: 1 }}
        exit={{ opacity: 0, scale: 0.95 }}
        transition={{ duration: 0.08 }}
        className="fixed bg-white dark:bg-gray-800 rounded-xl shadow-dropdown border border-gray-200 dark:border-gray-700 overflow-hidden z-50 min-w-[180px]"
        style={{ left: adjustedX, top: adjustedY }}
      >
        {items.map((item, i) =>
          'separator' in item ? (
            <div key={i} className="h-px bg-gray-100 dark:bg-gray-700 my-1" />
          ) : (
            <button
              key={item.label}
              onClick={() => { item.action(); onClose(); }}
              className={`w-full flex items-center gap-2.5 px-3 py-2 text-sm transition-colors ${
                item.danger
                  ? 'text-red-500 hover:bg-red-50 dark:hover:bg-red-900/20'
                  : 'text-gray-700 dark:text-gray-300 hover:bg-gray-100 dark:hover:bg-gray-700'
              }`}
            >
              <span className="flex-shrink-0">{item.icon}</span>
              {item.label}
            </button>
          )
        )}
      </motion.div>
    </>
  );
}
