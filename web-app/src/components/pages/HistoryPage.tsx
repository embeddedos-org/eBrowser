import React, { useState, useEffect } from 'react';
import { Clock, Search, Trash2, ExternalLink, X, Calendar } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion } from 'framer-motion';
import { historyDB, type HistoryEntry } from '@/utils/database';
import { useBrowserStore } from '@/store/browserStore';
import { formatURL } from '@/utils/url';
import { format, isToday, isYesterday, isThisWeek } from 'date-fns';

export default function HistoryPage() {
  const { t } = useTranslation();
  const store = useBrowserStore();
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [query, setQuery] = useState('');
  const [loading, setLoading] = useState(true);

  const loadHistory = async () => {
    setLoading(true);
    const data = query ? await historyDB.search(query) : await historyDB.getRecent(200);
    setHistory(data);
    setLoading(false);
  };

  useEffect(() => { loadHistory(); }, [query]);

  const handleDelete = async (id: number) => {
    await historyDB.deleteEntry(id);
    setHistory(h => h.filter(e => e.id !== id));
  };

  const handleClearAll = async () => {
    if (confirm('Clear all browsing history?')) {
      await historyDB.clear();
      setHistory([]);
    }
  };

  const handleNavigate = (url: string) => {
    const tab = store.getActiveTab();
    if (tab) store.navigateTo(url, tab.id);
  };

  const groupByDate = (entries: HistoryEntry[]) => {
    const groups: Record<string, HistoryEntry[]> = {};
    entries.forEach(entry => {
      const date = new Date(entry.visitedAt);
      let key: string;
      if (isToday(date)) key = 'Today';
      else if (isYesterday(date)) key = 'Yesterday';
      else if (isThisWeek(date)) key = 'This Week';
      else key = format(date, 'MMMM d, yyyy');
      if (!groups[key]) groups[key] = [];
      groups[key].push(entry);
    });
    return groups;
  };

  const groups = groupByDate(history);

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100 flex items-center gap-2">
          <Clock size={24} className="text-primary-500" />
          {t('history.title')}
        </h1>
        <button onClick={handleClearAll} className="btn-danger gap-2 text-sm">
          <Trash2 size={14} />
          {t('history.clearHistory')}
        </button>
      </div>

      <div className="relative mb-6">
        <Search size={16} className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" />
        <input
          type="text"
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          placeholder={t('history.searchHistory')}
          className="input pl-9"
        />
        {query && (
          <button onClick={() => setQuery('')} className="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400 hover:text-gray-600">
            <X size={14} />
          </button>
        )}
      </div>

      {loading ? (
        <div className="flex items-center justify-center py-12 text-gray-400">
          <div className="animate-spin w-6 h-6 border-2 border-primary-500 border-t-transparent rounded-full mr-3" />
          Loading...
        </div>
      ) : history.length === 0 ? (
        <div className="text-center py-12 text-gray-400">
          <Clock size={40} className="mx-auto mb-3 opacity-30" />
          <p>{t('history.noHistory')}</p>
        </div>
      ) : (
        <div className="space-y-6">
          {Object.entries(groups).map(([date, entries]) => (
            <div key={date}>
              <div className="section-title flex items-center gap-2 mb-2">
                <Calendar size={12} />
                {date}
              </div>
              <div className="space-y-0.5">
                {entries.map((entry) => (
                  <motion.div
                    key={entry.id}
                    initial={{ opacity: 0, x: -10 }}
                    animate={{ opacity: 1, x: 0 }}
                    className="group flex items-center gap-3 p-2.5 rounded-lg hover:bg-gray-100 dark:hover:bg-gray-800 transition-colors"
                  >
                    <div className="w-6 h-6 flex-shrink-0 flex items-center justify-center">
                      {entry.favicon ? (
                        <img src={entry.favicon} alt="" className="w-4 h-4 rounded-sm" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />
                      ) : (
                        <Clock size={14} className="text-gray-400" />
                      )}
                    </div>
                    <button
                      onClick={() => handleNavigate(entry.url)}
                      className="flex-1 min-w-0 text-left"
                    >
                      <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">{entry.title || entry.url}</div>
                      <div className="text-xs text-gray-400 truncate">{entry.url}</div>
                    </button>
                    <div className="flex items-center gap-2 opacity-0 group-hover:opacity-100 transition-opacity">
                      <span className="text-xs text-gray-400">
                        {format(new Date(entry.visitedAt), 'HH:mm')}
                      </span>
                      <button
                        onClick={() => window.open(entry.url, '_blank')}
                        className="icon-btn w-6 h-6"
                        title="Open in new tab"
                      >
                        <ExternalLink size={12} />
                      </button>
                      <button
                        onClick={() => entry.id && handleDelete(entry.id)}
                        className="icon-btn w-6 h-6 text-red-400 hover:text-red-600"
                        title={t('history.deleteEntry')}
                      >
                        <X size={12} />
                      </button>
                    </div>
                  </motion.div>
                ))}
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
