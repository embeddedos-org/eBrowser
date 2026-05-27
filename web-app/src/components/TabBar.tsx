import React, { useRef, useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { X, Plus, Lock, Volume2, VolumeX, Pin } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { useBrowserStore, type Tab } from '@/store/browserStore';
import { formatURL } from '@/utils/url';
import TabContextMenu from './TabContextMenu';

export default function TabBar() {
  const { t } = useTranslation();
  const { tabs, activeTabId, setActiveTab, closeTab, openTab, moveTab, settings } = useBrowserStore();
  const [contextMenu, setContextMenu] = useState<{ tab: Tab; x: number; y: number } | null>(null);
  const [dragOverIndex, setDragOverIndex] = useState<number | null>(null);
  const dragTabId = useRef<string | null>(null);

  const isIncognito = tabs.find(t => t.id === activeTabId)?.isIncognito ?? false;

  const handleDragStart = (e: React.DragEvent, tabId: string) => {
    dragTabId.current = tabId;
    e.dataTransfer.effectAllowed = 'move';
  };

  const handleDragOver = (e: React.DragEvent, index: number) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
    setDragOverIndex(index);
  };

  const handleDrop = (e: React.DragEvent, index: number) => {
    e.preventDefault();
    if (dragTabId.current) {
      moveTab(dragTabId.current, index);
      dragTabId.current = null;
    }
    setDragOverIndex(null);
  };

  const handleDragEnd = () => {
    dragTabId.current = null;
    setDragOverIndex(null);
  };

  const handleContextMenu = (e: React.MouseEvent, tab: Tab) => {
    e.preventDefault();
    setContextMenu({ tab, x: e.clientX, y: e.clientY });
  };

  const handleMiddleClick = (e: React.MouseEvent, tabId: string) => {
    if (e.button === 1) {
      e.preventDefault();
      closeTab(tabId);
    }
  };

  return (
    <div
      className={`flex items-end h-9 overflow-x-auto overflow-y-hidden no-scrollbar px-1 ${
        isIncognito
          ? 'bg-gray-900 dark:bg-gray-950'
          : 'bg-gray-100 dark:bg-gray-900'
      }`}
      style={{ minHeight: '36px' }}
    >
      {/* Pinned tabs first */}
      {tabs.filter(t => t.isPinned).map((tab, index) => (
        <TabItem
          key={tab.id}
          tab={tab}
          isActive={tab.id === activeTabId}
          isPinned
          index={index}
          dragOverIndex={dragOverIndex}
          onActivate={() => setActiveTab(tab.id)}
          onClose={() => closeTab(tab.id)}
          onDragStart={(e) => handleDragStart(e, tab.id)}
          onDragOver={(e) => handleDragOver(e, index)}
          onDrop={(e) => handleDrop(e, index)}
          onDragEnd={handleDragEnd}
          onContextMenu={(e) => handleContextMenu(e, tab)}
          onMiddleClick={(e) => handleMiddleClick(e, tab.id)}
        />
      ))}

      {/* Regular tabs */}
      {tabs.filter(t => !t.isPinned).map((tab, index) => {
        const realIndex = tabs.findIndex(t => t.id === tab.id);
        return (
          <TabItem
            key={tab.id}
            tab={tab}
            isActive={tab.id === activeTabId}
            isPinned={false}
            index={realIndex}
            dragOverIndex={dragOverIndex}
            onActivate={() => setActiveTab(tab.id)}
            onClose={() => closeTab(tab.id)}
            onDragStart={(e) => handleDragStart(e, tab.id)}
            onDragOver={(e) => handleDragOver(e, realIndex)}
            onDrop={(e) => handleDrop(e, realIndex)}
            onDragEnd={handleDragEnd}
            onContextMenu={(e) => handleContextMenu(e, tab)}
            onMiddleClick={(e) => handleMiddleClick(e, tab.id)}
          />
        );
      })}

      {/* New tab button */}
      <button
        onClick={() => openTab()}
        className="flex-shrink-0 flex items-center justify-center w-8 h-7 ml-1 rounded-md text-gray-500 dark:text-gray-400 hover:bg-gray-200 dark:hover:bg-gray-700 hover:text-gray-700 dark:hover:text-gray-200 transition-colors"
        title={t('browser.newTab')}
        aria-label={t('browser.newTab')}
      >
        <Plus size={16} />
      </button>

      {/* Context menu */}
      <AnimatePresence>
        {contextMenu && (
          <TabContextMenu
            tab={contextMenu.tab}
            x={contextMenu.x}
            y={contextMenu.y}
            onClose={() => setContextMenu(null)}
          />
        )}
      </AnimatePresence>
    </div>
  );
}

interface TabItemProps {
  tab: Tab;
  isActive: boolean;
  isPinned: boolean;
  index: number;
  dragOverIndex: number | null;
  onActivate: () => void;
  onClose: () => void;
  onDragStart: (e: React.DragEvent) => void;
  onDragOver: (e: React.DragEvent) => void;
  onDrop: (e: React.DragEvent) => void;
  onDragEnd: () => void;
  onContextMenu: (e: React.MouseEvent) => void;
  onMiddleClick: (e: React.MouseEvent) => void;
}

function TabItem({
  tab, isActive, isPinned, index, dragOverIndex,
  onActivate, onClose, onDragStart, onDragOver, onDrop, onDragEnd,
  onContextMenu, onMiddleClick
}: TabItemProps) {
  const { t } = useTranslation();
  const isDragOver = dragOverIndex === index;

  return (
    <motion.div
      layout
      initial={{ opacity: 0, scale: 0.9 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.9 }}
      transition={{ duration: 0.1 }}
      className={`
        group relative flex items-center gap-1.5 h-8 rounded-t-lg cursor-pointer
        border-t border-l border-r transition-colors duration-100 flex-shrink-0
        ${isPinned ? 'w-10 px-2 justify-center' : 'min-w-[120px] max-w-[220px] px-3'}
        ${isActive
          ? 'bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700 text-gray-900 dark:text-gray-100 z-10'
          : 'bg-gray-100 dark:bg-gray-900 border-transparent text-gray-600 dark:text-gray-400 hover:bg-gray-50 dark:hover:bg-gray-800'
        }
        ${isDragOver ? 'ring-2 ring-primary-500 ring-inset' : ''}
        ${tab.isIncognito ? 'bg-gray-800 dark:bg-gray-950 text-gray-200' : ''}
      `}
      onContextMenu={onContextMenu}
      onMouseDown={onMiddleClick}
      onClick={onActivate}
      title={tab.title}
      role="tab"
      aria-selected={isActive}
    >
      {/* Loading spinner or favicon */}
      <div className="flex-shrink-0 w-4 h-4">
        {tab.isLoading ? (
          <div className="w-4 h-4 border-2 border-primary-500 border-t-transparent rounded-full animate-spin" />
        ) : tab.favicon ? (
          <img
            src={tab.favicon}
            alt=""
            className="w-4 h-4 rounded-sm"
            onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }}
          />
        ) : (
          <div className="w-4 h-4 rounded-sm bg-gray-300 dark:bg-gray-600 flex items-center justify-center text-[8px] text-gray-500">
            {tab.title?.[0]?.toUpperCase() || '?'}
          </div>
        )}
      </div>

      {/* Tab title — hidden for pinned tabs */}
      {!isPinned && (
        <span className="flex-1 text-xs truncate leading-none">
          {tab.title || t('browser.newTab')}
        </span>
      )}

      {/* Indicators */}
      <div className="flex items-center gap-0.5 flex-shrink-0">
        {tab.isMuted && <VolumeX size={10} className="text-gray-400" />}
        {!tab.isMuted && tab.status === 'complete' && tab.url.includes('audio') && (
          <Volume2 size={10} className="text-primary-500" />
        )}
        {tab.isIncognito && <Lock size={10} className="text-purple-400" />}
        {isPinned && <Pin size={10} className="text-gray-400" />}
      </div>

      {/* Close button */}
      {!isPinned && (
        <button
          onClick={(e) => { e.stopPropagation(); onClose(); }}
          className={`
            flex-shrink-0 flex items-center justify-center w-4 h-4 rounded-full
            text-gray-400 hover:text-gray-600 dark:hover:text-gray-200
            hover:bg-gray-200 dark:hover:bg-gray-600
            transition-all duration-100
            ${isActive ? 'opacity-100' : 'opacity-0 group-hover:opacity-100'}
          `}
          title={t('browser.closeTab')}
          aria-label={t('browser.closeTab')}
        >
          <X size={10} />
        </button>
      )}

      {/* Active tab bottom border cover */}
      {isActive && (
        <div className="absolute bottom-[-1px] left-0 right-0 h-px bg-white dark:bg-gray-800" />
      )}
    </motion.div>
  );
}
