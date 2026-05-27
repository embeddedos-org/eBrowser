import React, { useState, useEffect } from 'react';
import { Search, Plus, X, Settings, MapPin, Wifi, WifiOff, Clock } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion, AnimatePresence } from 'framer-motion';
import { useBrowserStore } from '@/store/browserStore';
import { normalizeInput, getFaviconUrl } from '@/utils/url';
import { historyDB } from '@/utils/database';
import type { HistoryEntry } from '@/utils/database';

export default function NewTabPage() {
  const { t } = useTranslation();
  const store = useBrowserStore();
  const [searchQuery, setSearchQuery] = useState('');
  const [greeting, setGreeting] = useState('');
  const [recentHistory, setRecentHistory] = useState<HistoryEntry[]>([]);
  const [time, setTime] = useState(new Date());
  const [showCustomize, setShowCustomize] = useState(false);

  // Update clock
  useEffect(() => {
    const timer = setInterval(() => setTime(new Date()), 1000);
    return () => clearInterval(timer);
  }, []);

  // Set greeting based on time
  useEffect(() => {
    const hour = time.getHours();
    if (hour < 12) setGreeting('morning');
    else if (hour < 17) setGreeting('afternoon');
    else setGreeting('evening');
  }, [time]);

  // Load recent history
  useEffect(() => {
    historyDB.getRecent(8).then(setRecentHistory);
  }, []);

  const handleSearch = (e: React.FormEvent) => {
    e.preventDefault();
    if (!searchQuery.trim()) return;
    const url = normalizeInput(searchQuery);
    const activeTab = store.getActiveTab();
    if (activeTab) store.navigateTo(url, activeTab.id);
  };

  const formatTime = (date: Date) => {
    return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  };

  const formatDate = (date: Date) => {
    return date.toLocaleDateString([], { weekday: 'long', month: 'long', day: 'numeric' });
  };

  const isIncognito = store.getActiveTab()?.isIncognito ?? false;

  return (
    <div className={`w-full h-full overflow-auto ${isIncognito ? 'bg-gray-900' : 'bg-gradient-to-br from-slate-50 to-blue-50 dark:from-gray-900 dark:to-gray-800'}`}>
      <div className="max-w-4xl mx-auto px-6 py-12 flex flex-col items-center gap-10">

        {/* Clock and Date */}
        <motion.div
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          className="text-center"
        >
          <div className={`text-6xl font-light tracking-tight ${isIncognito ? 'text-white' : 'text-gray-800 dark:text-white'}`}>
            {formatTime(time)}
          </div>
          <div className={`text-lg mt-1 ${isIncognito ? 'text-gray-300' : 'text-gray-500 dark:text-gray-400'}`}>
            {formatDate(time)}
          </div>
          {store.gpsLocation && (
            <div className="flex items-center justify-center gap-1 mt-1 text-sm text-primary-500">
              <MapPin size={12} />
              <span>{store.gpsLocation.latitude.toFixed(2)}°, {store.gpsLocation.longitude.toFixed(2)}°</span>
            </div>
          )}
        </motion.div>

        {/* Greeting */}
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.1 }}
          className={`text-2xl font-medium ${isIncognito ? 'text-gray-200' : 'text-gray-700 dark:text-gray-200'}`}
        >
          {isIncognito ? '🕵️ Incognito Mode — Your browsing is private' : `Good ${greeting}!`}
        </motion.div>

        {/* Search bar */}
        <motion.form
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.15 }}
          onSubmit={handleSearch}
          className="w-full max-w-2xl"
        >
          <div className={`flex items-center gap-3 rounded-full px-5 py-3.5 shadow-lg ${
            isIncognito
              ? 'bg-gray-800 border border-gray-700'
              : 'bg-white dark:bg-gray-800 border border-gray-200 dark:border-gray-700'
          }`}>
            <Search size={20} className="text-gray-400 flex-shrink-0" />
            <input
              type="text"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder={t('newtab.searchPlaceholder')}
              className="flex-1 bg-transparent outline-none text-base text-gray-900 dark:text-gray-100 placeholder-gray-400"
              autoFocus
            />
            {searchQuery && (
              <button type="button" onClick={() => setSearchQuery('')} className="text-gray-400 hover:text-gray-600">
                <X size={16} />
              </button>
            )}
            <button type="submit" className="btn-primary rounded-full px-4 py-1.5 text-sm">
              {t('address.search')}
            </button>
          </div>
        </motion.form>

        {/* Top Sites */}
        {!isIncognito && (
          <motion.div
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.2 }}
            className="w-full"
          >
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-sm font-semibold text-gray-500 dark:text-gray-400 uppercase tracking-wider">
                {t('newtab.topSites')}
              </h2>
              <button
                onClick={() => setShowCustomize(!showCustomize)}
                className="text-xs text-primary-500 hover:text-primary-600 flex items-center gap-1"
              >
                <Settings size={12} />
                {t('newtab.customize')}
              </button>
            </div>
            <div className="grid grid-cols-4 sm:grid-cols-6 lg:grid-cols-8 gap-4">
              {store.pinnedSites.map((site, i) => (
                <motion.button
                  key={site.url}
                  initial={{ opacity: 0, scale: 0.8 }}
                  animate={{ opacity: 1, scale: 1 }}
                  transition={{ delay: 0.2 + i * 0.03 }}
                  onClick={() => {
                    const tab = store.getActiveTab();
                    if (tab) store.navigateTo(site.url, tab.id);
                  }}
                  className="flex flex-col items-center gap-2 p-3 rounded-xl hover:bg-white dark:hover:bg-gray-800 hover:shadow-md transition-all duration-150 group"
                >
                  <div className="w-10 h-10 rounded-xl bg-gray-100 dark:bg-gray-700 flex items-center justify-center overflow-hidden shadow-sm">
                    {site.favicon ? (
                      <img
                        src={site.favicon}
                        alt={site.title}
                        className="w-6 h-6"
                        onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }}
                      />
                    ) : (
                      <span className="text-lg font-bold text-gray-500">{site.title[0]}</span>
                    )}
                  </div>
                  <span className="text-xs text-gray-600 dark:text-gray-400 truncate w-full text-center">
                    {site.title}
                  </span>
                </motion.button>
              ))}
              <motion.button
                initial={{ opacity: 0, scale: 0.8 }}
                animate={{ opacity: 1, scale: 1 }}
                transition={{ delay: 0.4 }}
                onClick={() => {/* add site */}}
                className="flex flex-col items-center gap-2 p-3 rounded-xl hover:bg-white dark:hover:bg-gray-800 hover:shadow-md transition-all duration-150"
              >
                <div className="w-10 h-10 rounded-xl bg-gray-100 dark:bg-gray-700 flex items-center justify-center">
                  <Plus size={20} className="text-gray-400" />
                </div>
                <span className="text-xs text-gray-400">Add</span>
              </motion.button>
            </div>
          </motion.div>
        )}

        {/* Recent History */}
        {!isIncognito && recentHistory.length > 0 && (
          <motion.div
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: 0.3 }}
            className="w-full"
          >
            <h2 className="text-sm font-semibold text-gray-500 dark:text-gray-400 uppercase tracking-wider mb-4">
              {t('newtab.recentlyVisited')}
            </h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-2">
              {recentHistory.slice(0, 6).map((entry) => (
                <button
                  key={entry.id}
                  onClick={() => {
                    const tab = store.getActiveTab();
                    if (tab) store.navigateTo(entry.url, tab.id);
                  }}
                  className="flex items-center gap-3 p-3 rounded-lg bg-white dark:bg-gray-800 hover:bg-gray-50 dark:hover:bg-gray-750 border border-gray-100 dark:border-gray-700 transition-colors text-left"
                >
                  <div className="w-8 h-8 rounded-lg bg-gray-100 dark:bg-gray-700 flex items-center justify-center flex-shrink-0 overflow-hidden">
                    {entry.favicon ? (
                      <img src={entry.favicon} alt="" className="w-5 h-5" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />
                    ) : (
                      <Clock size={14} className="text-gray-400" />
                    )}
                  </div>
                  <div className="flex-1 min-w-0">
                    <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">{entry.title}</div>
                    <div className="text-xs text-gray-400 truncate">{entry.url}</div>
                  </div>
                </button>
              ))}
            </div>
          </motion.div>
        )}

        {/* Network status */}
        {store.networkStatus !== 'online' && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            className={`flex items-center gap-2 px-4 py-2 rounded-full text-sm ${
              store.networkStatus === 'offline'
                ? 'bg-red-100 dark:bg-red-900/20 text-red-600 dark:text-red-400'
                : 'bg-yellow-100 dark:bg-yellow-900/20 text-yellow-600 dark:text-yellow-400'
            }`}
          >
            {store.networkStatus === 'offline' ? <WifiOff size={14} /> : <Wifi size={14} />}
            {store.networkStatus === 'offline' ? t('common.offline') : t('common.slowConnection')}
          </motion.div>
        )}

        {/* eBrowser branding */}
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.5 }}
          className="text-center"
        >
          <div className={`text-xs ${isIncognito ? 'text-gray-500' : 'text-gray-400 dark:text-gray-600'}`}>
            eBrowser v2.0 — The World's Most Powerful Browser
          </div>
        </motion.div>
      </div>
    </div>
  );
}
