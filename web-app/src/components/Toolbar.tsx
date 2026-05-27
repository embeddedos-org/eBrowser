import React, { useState, useRef, useCallback, useEffect } from 'react';
import {
  ChevronLeft, ChevronRight, RotateCw, X, Home, Star, StarOff,
  Shield, ShieldAlert, ShieldOff, Lock, Unlock, Globe,
  BookOpen, Download, Settings, Menu, Mic, Camera,
  SlidersHorizontal, Share2, Printer, Maximize2, EyeOff,
  Zap, Search, MapPin
} from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion, AnimatePresence } from 'framer-motion';
import { useBrowserStore } from '@/store/browserStore';
import { normalizeInput, getSecurityLevel, isInternalPage } from '@/utils/url';
import { bookmarkDB } from '@/utils/database';
import AddressBarDropdown from './AddressBarDropdown';
import BrowserMenu from './BrowserMenu';

export default function Toolbar() {
  const { t } = useTranslation();
  const store = useBrowserStore();
  const activeTab = store.getActiveTab();

  const [addressValue, setAddressValue] = useState(activeTab?.displayUrl ?? '');
  const [isFocused, setIsFocused] = useState(false);
  const [showDropdown, setShowDropdown] = useState(false);
  const [showMenu, setShowMenu] = useState(false);
  const [isBookmarked, setIsBookmarked] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);
  const dropdownRef = useRef<HTMLDivElement>(null);

  // Sync address bar with active tab URL
  useEffect(() => {
    if (!isFocused && activeTab) {
      setAddressValue(activeTab.displayUrl || activeTab.url);
    }
  }, [activeTab?.url, activeTab?.displayUrl, isFocused]);

  // Check bookmark status
  useEffect(() => {
    if (activeTab?.url && !isInternalPage(activeTab.url)) {
      bookmarkDB.isBookmarked(activeTab.url).then(setIsBookmarked);
    } else {
      setIsBookmarked(false);
    }
  }, [activeTab?.url]);

  const handleNavigate = useCallback((value: string) => {
    const url = normalizeInput(value);
    if (activeTab) {
      store.navigateTo(url, activeTab.id);
    }
    setShowDropdown(false);
    inputRef.current?.blur();
  }, [activeTab, store]);

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      handleNavigate(addressValue);
    } else if (e.key === 'Escape') {
      setAddressValue(activeTab?.displayUrl ?? '');
      setShowDropdown(false);
      inputRef.current?.blur();
    } else if (e.key === 'ArrowDown' && showDropdown) {
      // Focus first dropdown item
      const firstItem = dropdownRef.current?.querySelector('[data-dropdown-item]') as HTMLElement;
      firstItem?.focus();
    }
  };

  const handleFocus = () => {
    setIsFocused(true);
    setShowDropdown(true);
    // Select all text on focus
    setTimeout(() => inputRef.current?.select(), 0);
  };

  const handleBlur = (e: React.FocusEvent) => {
    // Don't blur if clicking inside dropdown
    if (dropdownRef.current?.contains(e.relatedTarget as Node)) return;
    setIsFocused(false);
    setShowDropdown(false);
    if (activeTab) setAddressValue(activeTab.displayUrl || activeTab.url);
  };

  const handleBookmark = async () => {
    if (!activeTab || isInternalPage(activeTab.url)) return;
    if (isBookmarked) {
      const bm = await bookmarkDB.getByUrl(activeTab.url);
      if (bm?.id) {
        await bookmarkDB.delete(bm.id);
        setIsBookmarked(false);
        store.addToast({ type: 'info', message: t('bookmarks.bookmarkRemoved') });
      }
    } else {
      await bookmarkDB.add({ url: activeTab.url, title: activeTab.title, favicon: activeTab.favicon });
      setIsBookmarked(true);
      store.addToast({ type: 'success', message: t('bookmarks.bookmarkAdded') });
    }
  };

  const securityLevel = activeTab ? getSecurityLevel(activeTab.url) : 'local';

  const SecurityIcon = () => {
    switch (securityLevel) {
      case 'secure': return <Lock size={13} className="security-indicator-secure" />;
      case 'warning': return <Unlock size={13} className="security-indicator-warning" />;
      case 'danger': return <ShieldOff size={13} className="security-indicator-danger" />;
      case 'local': return <Globe size={13} className="text-gray-400" />;
      case 'extension': return <Zap size={13} className="text-purple-500" />;
      default: return <Globe size={13} className="text-gray-400" />;
    }
  };

  const isIncognito = activeTab?.isIncognito ?? false;

  return (
    <div
      className={`flex items-center gap-1 px-2 h-10 border-b border-gray-200 dark:border-gray-700 ${
        isIncognito
          ? 'bg-gray-800 dark:bg-gray-900'
          : 'bg-toolbar dark:bg-toolbar-dark'
      }`}
    >
      {/* Navigation buttons */}
      <div className="flex items-center gap-0.5">
        <button
          onClick={() => store.goBack()}
          disabled={!activeTab?.canGoBack}
          className="icon-btn disabled:opacity-30 disabled:cursor-not-allowed"
          title={`${t('browser.back')} (Alt+←)`}
          aria-label={t('browser.back')}
        >
          <ChevronLeft size={18} />
        </button>
        <button
          onClick={() => store.goForward()}
          disabled={!activeTab?.canGoForward}
          className="icon-btn disabled:opacity-30 disabled:cursor-not-allowed"
          title={`${t('browser.forward')} (Alt+→)`}
          aria-label={t('browser.forward')}
        >
          <ChevronRight size={18} />
        </button>
        <button
          onClick={() => activeTab?.isLoading ? store.updateTab(activeTab.id, { isLoading: false, status: 'idle' }) : store.reload()}
          className="icon-btn"
          title={activeTab?.isLoading ? t('browser.stop') : `${t('browser.reload')} (Ctrl+R)`}
          aria-label={activeTab?.isLoading ? t('browser.stop') : t('browser.reload')}
        >
          {activeTab?.isLoading
            ? <X size={16} />
            : <RotateCw size={16} />
          }
        </button>
        <button
          onClick={() => store.navigateTo(store.settings.homepage)}
          className="icon-btn"
          title={t('browser.home')}
          aria-label={t('browser.home')}
        >
          <Home size={16} />
        </button>
      </div>

      {/* Address bar */}
      <div className="flex-1 relative">
        <div
          className={`address-bar ${isFocused ? 'ring-2 ring-primary-500 bg-white dark:bg-gray-600 shadow-md' : ''} ${
            isIncognito ? 'bg-gray-700 dark:bg-gray-800' : ''
          }`}
        >
          {/* Security indicator */}
          <button
            className="flex-shrink-0 flex items-center p-0.5 rounded hover:bg-gray-200 dark:hover:bg-gray-600 transition-colors"
            title={securityLevel === 'secure' ? t('address.secure') : t('address.notSecure')}
            onClick={() => {/* show security info */}}
          >
            <SecurityIcon />
          </button>

          {/* GPS indicator when location is active */}
          {store.gpsLocation && (
            <MapPin size={12} className="text-primary-500 flex-shrink-0" />
          )}

          {/* Input */}
          <input
            ref={inputRef}
            id="address-bar-input"
            type="text"
            value={isFocused ? addressValue : (activeTab?.displayUrl || activeTab?.url || '')}
            onChange={(e) => setAddressValue(e.target.value)}
            onKeyDown={handleKeyDown}
            onFocus={handleFocus}
            onBlur={handleBlur}
            placeholder={t('address.placeholder')}
            className="flex-1 bg-transparent outline-none text-sm text-gray-900 dark:text-gray-100 placeholder-gray-400 dark:placeholder-gray-500"
            spellCheck={false}
            autoComplete="off"
            autoCorrect="off"
            autoCapitalize="off"
            aria-label={t('address.placeholder')}
          />

          {/* Loading progress */}
          {activeTab?.isLoading && (
            <div className="absolute bottom-0 left-0 right-0 h-0.5 bg-gray-200 dark:bg-gray-600 rounded-full overflow-hidden">
              <div className="h-full bg-primary-500 loading-bar rounded-full" />
            </div>
          )}

          {/* Clear button when focused */}
          {isFocused && addressValue && (
            <button
              onMouseDown={(e) => { e.preventDefault(); setAddressValue(''); }}
              className="flex-shrink-0 icon-btn w-5 h-5"
              aria-label="Clear"
            >
              <X size={12} />
            </button>
          )}

          {/* Bookmark star */}
          {!isFocused && activeTab && !isInternalPage(activeTab.url) && (
            <button
              onClick={handleBookmark}
              className={`flex-shrink-0 flex items-center p-0.5 rounded hover:bg-gray-200 dark:hover:bg-gray-600 transition-colors ${
                isBookmarked ? 'text-yellow-500' : 'text-gray-400 hover:text-gray-600 dark:hover:text-gray-200'
              }`}
              title={isBookmarked ? t('browser.removeBookmark') : t('browser.addBookmark')}
            >
              {isBookmarked ? <Star size={14} fill="currentColor" /> : <Star size={14} />}
            </button>
          )}
        </div>

        {/* Dropdown suggestions */}
        <AnimatePresence>
          {showDropdown && isFocused && (
            <div ref={dropdownRef}>
              <AddressBarDropdown
                query={addressValue}
                onSelect={handleNavigate}
                onClose={() => setShowDropdown(false)}
              />
            </div>
          )}
        </AnimatePresence>
      </div>

      {/* Right side actions */}
      <div className="flex items-center gap-0.5">
        {/* Reader mode */}
        {activeTab && !isInternalPage(activeTab.url) && (
          <button
            onClick={() => store.updateTab(activeTab.id, { readingMode: !activeTab.readingMode })}
            className={`icon-btn ${activeTab.readingMode ? 'text-primary-500' : ''}`}
            title={t('browser.readingMode')}
          >
            <BookOpen size={16} />
          </button>
        )}

        {/* Downloads */}
        <button
          onClick={() => store.toggleDownloadsPanel()}
          className="icon-btn"
          title={`${t('browser.downloads')} (Ctrl+J)`}
        >
          <Download size={16} />
        </button>

        {/* Extensions */}
        <button
          onClick={() => store.setSidebarPanel('extensions')}
          className="icon-btn"
          title={t('browser.extensions')}
        >
          <Zap size={16} />
        </button>

        {/* Main menu */}
        <div className="relative">
          <button
            onClick={() => setShowMenu(!showMenu)}
            className="icon-btn"
            title="Menu"
            aria-label="Browser menu"
          >
            <Menu size={18} />
          </button>
          <AnimatePresence>
            {showMenu && (
              <BrowserMenu onClose={() => setShowMenu(false)} />
            )}
          </AnimatePresence>
        </div>
      </div>
    </div>
  );
}
